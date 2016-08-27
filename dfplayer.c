/*******************************************************************************
 * Copyright (C) 2014 DFRobot						       *
 *									       *
 * DFPlayer_Mini_Mp3, This library provides a quite complete function for      * 
 * DFPlayer mini mp3 module.                                                   *
 * www.github.com/dfrobot/DFPlayer_Mini_Mp3 (github as default source provider)*
 *  DFRobot-A great source for opensource hardware and robot.                  *
 *                                                                             *
 * This file is part of the DFplayer_Mini_Mp3 library.                         *
 *                                                                             *
 * DFPlayer_Mini_Mp3 is free software: you can redistribute it and/or          *
 * modify it under the terms of the GNU Lesser General Public License as       *
 * published by the Free Software Foundation, either version 3 of              *
 * the License, or any later version.                                          *
 *                                                                             *
 * DFPlayer_Mini_Mp3 is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * DFPlayer_Mini_Mp3 is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public            *
 * License along with DFPlayer_Mini_Mp3. If not, see                           *
 * <http://www.gnu.org/licenses/>.                                             *
 ******************************************************************************/

/*
 *	name:		DFPlayer_Mini_Mp3
 *	version:	1.0
 *	Author:		lisper <lisper.li@dfrobot.com>
 *	Date:		2014-05-22
 *	Description:	mp3 library for DFPlayer mini board
 *			note: mp3 file must put into mp3 folder in your tf card
 */

#include "dfplayer.h"
#include "usart.h"

extern u8 send_buf[10];
extern u8 recv_buf[10];

static u8 recv_buf_idx = 0;

//
static void fill_uint16_bigend (u8 *thebuf, u16 data) {
	*thebuf =	(u8)(data>>8);
	*(thebuf+1) =	(u8)data;
}


//calc checksum (1~6 byte)
u16 mp3_get_checksum (u8 *thebuf) {
	u16 sum = 0;
	for (int i=1; i<7; i++) {
		sum += thebuf[i];
	}
	return -sum;
}

//fill checksum to send_buf (7~8 byte)
void mp3_fill_checksum () {
	u16 checksum = mp3_get_checksum (send_buf);
	fill_uint16_bigend (send_buf+7, checksum);
}

//
void send () {
	for (int i=0; i<10; i++) {
        UsartPutc (send_buf[i]);
	}
}

//
void mp3_send_cmd_arg (u8 cmd, u16 arg) {
	send_buf[3] = cmd;
	fill_uint16_bigend ((send_buf+5), arg);
	mp3_fill_checksum ();
	send ();
}

//
void mp3_send_cmd (u8 cmd) {
	send_buf[3] = cmd;
	fill_uint16_bigend ((send_buf+5), 0);
	mp3_fill_checksum ();
	send ();
}

//
void mp3_send_cmd_reply (u8 cmd) {
	send_buf[3] = cmd;
    send_buf[4] = 1;
	fill_uint16_bigend ((send_buf+5), 0);
	mp3_fill_checksum ();
	send ();
}

//
void mp3_play_physical_num (u16 num) {
	mp3_send_cmd_arg (0x03, num);
}

//
void mp3_play_physical () {
	mp3_send_cmd (0x03);
}

//
void mp3_next () {
	mp3_send_cmd (0x01);
}

//
void mp3_prev () {
	mp3_send_cmd (0x02);
}

//0x06 set volume 0-30
void mp3_set_volume (u16 volume) {
	mp3_send_cmd_arg (0x06, volume);
}

//0x07 set EQ0/1/2/3/4/5    Normal/Pop/Rock/Jazz/Classic/Bass
void mp3_set_EQ (u16 eq) {
	mp3_send_cmd_arg (0x07, eq);
}

//0x08 Specify playback mode (0/1/2/3)    Repeat/folder repeat/single repeat/ random
void mp3_set_play_mode (u16 mode) {
	mp3_send_cmd_arg (0x08, mode);
}

//0x09 set device 1/2/3/4/5 U/SD/AUX/SLEEP/FLASH
void mp3_set_device (u16 device) {
	mp3_send_cmd_arg (0x09, device);
}

//
void mp3_sleep () {
	mp3_send_cmd (0x0a);
}

//
void mp3_reset () {
	mp3_send_cmd (0x0c);
}

//
void mp3_play () {
	mp3_send_cmd (0x0d);
}

//
void mp3_pause () {
	mp3_send_cmd (0x0e);
}

//
void mp3_stop () {
	mp3_send_cmd (0x16);
}

//play mp3 file in mp3 folder in your tf card
void mp3_play_num (u16 num) {
	mp3_send_cmd_arg (0x12, num);
}

//
void mp3_get_state () {
	mp3_send_cmd (0x42);
}

//
void mp3_get_volume () {
	mp3_send_cmd (0x43);
}


//
void mp3_get_u_sum () {
	mp3_send_cmd (0x47);
}

//
void mp3_get_tf_sum () {
	mp3_send_cmd (0x48);
}

//
void mp3_get_flash_sum () {
	mp3_send_cmd (0x49);
}


//
void mp3_get_tf_current () {
	mp3_send_cmd (0x4c);
}

//
void mp3_get_u_current () {
	mp3_send_cmd (0x4b);
}


//
void mp3_get_flash_current () {
	mp3_send_cmd (0x4d);
}

//
void mp3_single_loop (u8 state) {
	mp3_send_cmd_arg (0x19, !state);
}

//add 
void mp3_single_play (u16 num) {
	mp3_play_num (num);
	__delay_ms (10);
	mp3_single_loop (1); 
	//mp3_send_cmd (0x19, !state);
}

//
void mp3_DAC (u8 state) {
	mp3_send_cmd_arg (0x1a, !state);
}

//
void mp3_random_play () {
	mp3_send_cmd (0x18);
}

u8 mp3_get_num_files() {
    recv_buf_idx = 0;
    mp3_send_cmd(0x48);
    
    while (recv_buf_idx < 10) {
    }
    
//    for (int i = 0; i < recv_buf_idx; ++ i) {
//        eeprom_write(2 + i, recv_buf[i]);
//    }
    
    return recv_buf[6];
}

void mp3_get_num_files_async() {
    recv_buf_idx = 0;
    mp3_send_cmd(0x48);
}

u8 mp3_check_for_result() {
    return recv_buf_idx >= 10;
}

u8 mp3_get_result() {
    return recv_buf[6];
}

void mp3_wait_for_message() {
    recv_buf_idx = 0;
    while (recv_buf_idx < 10) {
    }  
}

void mp3_on_byte_received(u8 byte) {
    if (recv_buf_idx >= 10) {
        return;
    }
    
    recv_buf[recv_buf_idx] = byte;
    ++ recv_buf_idx;
}