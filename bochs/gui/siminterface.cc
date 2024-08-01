/////////////////////////////////////////////////////////////////////////
// $Id: siminterface.cc 12698 2015-03-29 14:27:32Z vruppert $
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2002-2015  The Bochs Project
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
//
/////////////////////////////////////////////////////////////////////////
  
// See siminterface.h for description of the siminterface concept.
// Basically, the siminterface is visible from both the simulator and
// the configuration user interface, and allows them to talk to each other.

#include "param_names.h"
#include "bochs.h"

bx_simulator_interface_c *SIM = NULL;
bx_list_c *root_param = NULL;




class bx_real_sim_c : public bx_simulator_interface_c {

  int init_done;
  int enabled;

  int exit_code;
  unsigned param_id;

public:
  bx_real_sim_c();
  virtual ~bx_real_sim_c() {}

  virtual int get_init_done() { return init_done; }
  virtual int set_init_done(int n) { init_done = n; return 0;}
  virtual void reset_all_param();
  // new param methods
  virtual bx_param_c *get_param(const char *pname, bx_param_c *base=NULL);
  virtual bx_param_num_c *get_param_num(const char *pname, bx_param_c *base=NULL);
  virtual bx_param_string_c *get_param_string(const char *pname, bx_param_c *base=NULL);
  virtual bx_param_bool_c *get_param_bool(const char *pname, bx_param_c *base=NULL);
  virtual bx_param_enum_c *get_param_enum(const char *pname, bx_param_c *base=NULL);
  virtual Bit32u gen_param_id() { return param_id++; }


  virtual bx_list_c *get_bochs_root() {
    return (bx_list_c*)get_param("bochs", NULL);
  }

private:

};

// recursive function to find parameters from the path
static bx_param_c *find_param(const char *full_pname, const char *rest_of_pname, bx_param_c *base)
{
  const char *from = rest_of_pname;
  char component[BX_PATHNAME_LEN];
  char *to = component;
  // copy the first piece of pname into component, stopping at first separator
  // or at the end of the string
  while (*from != 0 && *from != '.') {
    *to = *from;
    to++;
    from++;
  }
  *to = 0;
  if (!component[0]) {
    BX_PANIC(("find_param: found empty component in parameter name '%s'", full_pname));
    // or does that mean that we're done?
  }
  if (base->get_type() != BXT_LIST) {
    BX_PANIC(("find_param: base was not a list!"));
  }
  BX_DEBUG(("searching for component '%s' in list '%s'", component, base->get_name()));

  // find the component in the list.
  bx_list_c *list = (bx_list_c *)base;
  bx_param_c *child = list->get_by_name(component);
  // if child not found, there is nothing else that can be done. return NULL.
  if (child == NULL) return NULL;
  if (from[0] == 0) {
    // that was the end of the path, we're done
    return child;
  }
  // continue parsing the path
  BX_ASSERT(from[0] == '.');
  from++;  // skip over the separator
  return find_param(full_pname, from, child);
}

bx_param_c *bx_real_sim_c::get_param(const char *pname, bx_param_c *base)
{
  if (base == NULL)
    base = root_param;
  // to access top level object, look for parameter "."
  if (pname[0] == '.' && pname[1] == 0)
    return base;
  return find_param(pname, pname, base);
}

bx_param_num_c *bx_real_sim_c::get_param_num(const char *pname, bx_param_c *base)
{
  bx_param_c *gen = get_param(pname, base);
  if (gen==NULL) {
    BX_ERROR(("get_param_num(%s) could not find a parameter", pname));
    return NULL;
  }
  int type = gen->get_type();
  if (type == BXT_PARAM_NUM || type == BXT_PARAM_BOOL || type == BXT_PARAM_ENUM)
    return (bx_param_num_c *)gen;
  BX_ERROR(("get_param_num(%s) could not find an integer parameter with that name", pname));
  return NULL;
}

bx_param_string_c *bx_real_sim_c::get_param_string(const char *pname, bx_param_c *base)
{
  bx_param_c *gen = get_param(pname, base);
  if (gen==NULL) {
    BX_ERROR(("get_param_string(%s) could not find a parameter", pname));
    return NULL;
  }
  if (gen->get_type() == BXT_PARAM_STRING)
    return (bx_param_string_c *)gen;
  BX_ERROR(("get_param_string(%s) could not find an integer parameter with that name", pname));
  return NULL;
}

bx_param_bool_c *bx_real_sim_c::get_param_bool(const char *pname, bx_param_c *base)
{
  bx_param_c *gen = get_param(pname, base);
  if (gen==NULL) {
    BX_ERROR(("get_param_bool(%s) could not find a parameter", pname));
    return NULL;
  }
  if (gen->get_type () == BXT_PARAM_BOOL)
    return (bx_param_bool_c *)gen;
  BX_ERROR(("get_param_bool(%s) could not find a bool parameter with that name", pname));
  return NULL;
}

bx_param_enum_c *bx_real_sim_c::get_param_enum(const char *pname, bx_param_c *base)
{
  bx_param_c *gen = get_param(pname, base);
  if (gen==NULL) {
    BX_ERROR(("get_param_enum(%s) could not find a parameter", pname));
    return NULL;
  }
  if (gen->get_type() == BXT_PARAM_ENUM)
    return (bx_param_enum_c *)gen;
  BX_ERROR(("get_param_enum(%s) could not find a enum parameter with that name", pname));
  return NULL;
}

void bx_init_siminterface()
{
  if (SIM == NULL) {
    SIM = new bx_real_sim_c();
  }
  if (root_param == NULL) {
    root_param = new bx_list_c(NULL,
      "bochs",
      "list of top level bochs parameters"
      );
  }
}

bx_real_sim_c::bx_real_sim_c()
{

  enabled = 1;
  init_done = 0;
  exit_code = 0;
  param_id = BXP_NEW_PARAM_ID;

}

void bx_real_sim_c::reset_all_param()
{
  bx_reset_options();
}


