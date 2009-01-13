/* Jedec .jed file parser

Copyright (C) Uwe Bonnes 2009

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

/*
 * Using a slightly corrected version from IPAL libjedec
 * Copyright (c) 2000 Stephen Williams (steve@icarus.com)
 */ 

#include <stdlib.h>

#include "jedecfile.h"
#include "io_exception.h"

static unsigned char*allocate_fusemap(unsigned size)
{
  unsigned char*ptr = (unsigned char*) calloc((size+7) / 8, 1);
      return ptr;
}

struct jedec_data {
      char*design_specification;

      unsigned fuse_count;
      unsigned pin_count;
      unsigned vector_count;
      unsigned checksum;
      unsigned char fuse_default;

      unsigned char*fuse_list;
};
typedef struct jedec_data *jedec_data_t;

int jedec_get_fuse(jedec_data_t jed, unsigned idx)
{
      unsigned byte, bit;
      if(idx >= jed->fuse_count);
      throw  io_exception(std::string("jedec_get_fuse"));

      byte = idx / 8;
      bit  = idx % 8;
      return (jed->fuse_list[byte] & (1 << bit))? 1 : 0;
}

void jedec_set_fuse(jedec_data_t jed, unsigned idx, int blow)
{
      unsigned byte, bit;
      if(idx >= jed->fuse_count)
	throw  io_exception(std::string("jedec_set_fuse"));

      byte = idx / 8;
      bit  = idx % 8;

      if (blow)
            jed->fuse_list[byte] |=  (1 << bit);
      else
            jed->fuse_list[byte] &= ~(1 << bit);
}

struct state_mach {
      jedec_data_t jed;

      void (*state)(int ch, struct state_mach*m);

      unsigned int checksum;
      union {
            struct {
                  unsigned cur_fuse;
            } l;
      } m;
};

static void m_startup(int ch, struct state_mach*m);
static void m_base(int ch, struct state_mach*m);
static void m_C(int ch, struct state_mach*m);
static void m_L(int ch, struct state_mach*m);
static void m_Lfuse(int ch, struct state_mach*m);
static void m_Q(int ch, struct state_mach*m);
static void m_QF(int ch, struct state_mach*m);
static void m_QP(int ch, struct state_mach*m);
static void m_skip(int ch, struct state_mach*m);

static void m_startup(int ch, struct state_mach*m)
{
      switch (ch) {
          case '\002':
            m->state = m_base;
            break;

          default:
            break;
      }
}

static void m_base(int ch, struct state_mach*m)
{
      if (isspace(ch))
            return;

      switch (ch) {
          case 'L':
            m->state = m_L;
            m->m.l.cur_fuse = 0;
            break;
          case 'Q':
            m->state = m_Q;
            break;
          case 'C':
            m->state = m_C;
            m->jed->checksum = 0;
            break;
          default:
            m->state = m_skip;
            break;
      }
}

static void m_C(int ch, struct state_mach*m)
{
      if (isspace(ch))
            return;

      if (ch == '*') {
            m->state = m_base;
            return;
      }

      if(ch >='0' && ch <='9')
        {
          m->jed->checksum <<=4;
          m->jed->checksum += ch - '0';
          return;
        }
      ch &= 0x5f;
      if(ch >='A' && ch <= 'F')
        {
          m->jed->checksum <<=4;
          m->jed->checksum += ch - '7';
          return;
        }

      printf("Dangling '%c' 0x%02x\n", ch, ch);
      fflush(stdout);
      throw  io_exception(std::string("m_C"));
 }  
static void m_L(int ch, struct state_mach*m)
{
      if (isdigit(ch)) {
            m->m.l.cur_fuse *= 10;
            m->m.l.cur_fuse += ch - '0';
            return;
      }

      if (isspace(ch)) {
            m->state = m_Lfuse;
            return;
      }

      if (ch == '*') {
            m->state = m_base;
            return;
      }

      m->state = 0;
}

static void m_Lfuse(int ch, struct state_mach*m)
{
      switch (ch) {

          case '*':
            m->state = m_base;
            break;

          case '0':
            jedec_set_fuse(m->jed, m->m.l.cur_fuse, 0);
            m->m.l.cur_fuse += 1;
            break;

          case '1':
            jedec_set_fuse(m->jed, m->m.l.cur_fuse, 1);
            m->m.l.cur_fuse += 1;
            break;

          case ' ':
          case '\n':
          case '\r':
            break;

          default:
            m->state = 0;
            break;
      }
}

static void m_Q(int ch, struct state_mach*m)
{
      switch (ch) {
          case 'F':
            if (m->jed->fuse_count != 0) {
                  m->state = 0;
                  return;
            }
            m->state = m_QF;
            m->jed->fuse_count = 0;
            break;
          case 'P':
            if (m->jed->pin_count != 0) {
                  m->state = 0;
                  return;
            }
            m->state = m_QP;
            m->jed->pin_count = 0;
            break;
          default:
            m->state = m_skip;
            break;
      }
}

static void m_QF(int ch, struct state_mach*m)
{
      if (isspace(ch))
            return;

      if (isdigit(ch)) {
            m->jed->fuse_count *= 10;
            m->jed->fuse_count += ch - '0';
            return;
      }

      if (ch == '*') {
            m->state = m_base;
            m->jed->fuse_list = allocate_fusemap(m->jed->fuse_count);
            return;
      }

      throw  io_exception(std::string("m_QF"));
}

static void m_QP(int ch, struct state_mach*m)
{
      if (isspace(ch))
            return;

      if (isdigit(ch)) {
            m->jed->pin_count *= 10;
            m->jed->pin_count += ch - '0';
            return;
      }

      if (ch == '*') {
            m->state = m_base;
            return;
      }

      throw  io_exception(std::string("m_QP"));
}

static void m_skip(int ch, struct state_mach*m)
{
      switch (ch) {
          case '*':
            m->state = m_base;
            break;

          default:
            break;
      }
}

JedecFile::JedecFile(char const * fname)
  : fuse_count(0), fuse_list(0), Error(false), logfile(stderr) 
{
  int ch;
  struct state_mach m;

  FILE *const  fp=fopen(fname,"rb");
  if(!fp)  throw  io_exception(std::string("Cannot open file ") + fname);
  
  m.jed = (jedec_data_t)calloc(1, sizeof(struct jedec_data));
  m.state = m_startup;
  while ((ch = fgetc(fp)) != EOF) {
    m.state(ch, &m);
    if (m.state == 0) {
      /* Some sort of error happened. */
      throw  io_exception(std::string("Cannot open file ") + fname);
    }
  }
  fuse_count = m.jed->fuse_count;
  checksum =  m.jed->checksum;
  fuse_list = m.jed->fuse_list;
  //printf("%d Pins\n", m.jed->pin_count);
}
  
 
