/////////////////////////////////////////////////////////////////////////
<<<<<<< HEAD
// $Id: usb_msd.h 12494 2014-09-29 17:48:30Z vruppert $
=======
// $Id: usb_msd.h 12865 2015-10-22 18:13:59Z vruppert $
>>>>>>> version-2.6.9
/////////////////////////////////////////////////////////////////////////
//
//  USB mass storage device support (ported from QEMU)
//
//  Copyright (c) 2006 CodeSourcery.
//  Written by Paul Brook
<<<<<<< HEAD
//  Copyright (C) 2009-2014  The Bochs Project
=======
//  Copyright (C) 2009-2015  The Bochs Project
>>>>>>> version-2.6.9
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
/////////////////////////////////////////////////////////////////////////

#ifndef BX_IODEV_USB_MSD_H
#define BX_IODEV_USB_MSD_H

class device_image_t;
class cdrom_base_c;
class scsi_device_t;

class usb_msd_device_c : public usb_device_c {
public:
  usb_msd_device_c(usbdev_type type, const char *filename);
  virtual ~usb_msd_device_c(void);

  virtual bx_bool init();
  virtual bx_bool set_option(const char *option);
  virtual const char* get_info();
<<<<<<< HEAD
=======
  virtual void runtime_config(void);
  void restore_handler(bx_list_c *conf);
>>>>>>> version-2.6.9

  virtual void handle_reset();
  virtual int handle_control(int request, int value, int index, int length, Bit8u *data);
  virtual int handle_data(USBPacket *p);
  virtual void register_state_specific(bx_list_c *parent);
  virtual void cancel_packet(USBPacket *p);
  bx_bool set_inserted(bx_bool value);
  bx_bool get_inserted();
<<<<<<< HEAD

protected:
  void copy_data();
  void send_status();
=======
  bx_bool get_locked();

protected:
  void copy_data();
  void send_status(USBPacket *p);
>>>>>>> version-2.6.9
  static void usb_msd_command_complete(void *this_ptr, int reason, Bit32u tag, Bit32u arg);
  void command_complete(int reason, Bit32u tag, Bit32u arg);

private:
  struct {
<<<<<<< HEAD
    Bit8u mode;
    Bit32u scsi_len;
    Bit8u *scsi_buf;
    Bit32u usb_len;
    Bit8u *usb_buf;
    Bit32u data_len;
    Bit32u residue;
    Bit32u tag;
    int result;
=======
    // members set in constructor / init
>>>>>>> version-2.6.9
    Bit8u image_mode;
    device_image_t *hdimage;
    cdrom_base_c *cdrom;
    scsi_device_t *scsi_dev;
<<<<<<< HEAD
    USBPacket *packet;
=======
>>>>>>> version-2.6.9
    bx_list_c *sr_list;
    const char *fname;
    bx_list_c *config;
    char info_txt[BX_PATHNAME_LEN];
    char journal[BX_PATHNAME_LEN]; // undoable / volatile disk only
    int size; // VVFAT disk only
<<<<<<< HEAD
  } s;

  static const char *cd_param_string_handler(bx_param_string_c *param, int set,
                                             const char *oldval, const char *val, int maxlen);
  static Bit64s cd_param_handler(bx_param_c *param, int set, Bit64s val);
=======
    // members handled by runtime config
    bx_bool status_changed;
    // members handled by save/restore
    Bit8u mode;
    Bit32u scsi_len;
    Bit32u usb_len;
    Bit32u data_len;
    Bit32u residue;
    Bit32u tag;
    int result;
    // members not handled by save/restore
    Bit8u *scsi_buf;
    Bit8u *usb_buf;
    USBPacket *packet;
  } s;

  static const char *cdrom_path_handler(bx_param_string_c *param, int set,
                                             const char *oldval, const char *val, int maxlen);
  static Bit64s cdrom_status_handler(bx_param_c *param, int set, Bit64s val);
>>>>>>> version-2.6.9
};

#endif
