/**
 * \file
 *
 * \brief ATXmega128A1 NVM Register Definition
 *
 * Copyright (C) 2009 Atmel Corporation. All rights reserved.
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 * Atmel AVR product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */
#ifndef ATXMEGA128A1_NVM_REGS
#define ATXMEGA128A1_NVM_REGS

#define XNVM_FLASH_BASE                 0x0800000 //!< Adress where the flash starts.
#define XNVM_EEPROM_BASE                0x08C0000 //!< Address where eeprom starts.
#define XNVM_FUSE_BASE                  0x08F0020 //!< Address where fuses start.
#define XNVM_DATA_BASE                  0x1000000 //!< Address where data region starts.
#define XNVM_APPL_BASE            XNVM_FLASH_BASE //!< Addres where application section starts.
#define XNVM_CALIBRATION_BASE          0x008E0200 //!< Address where calibration row starts.
#define XNVM_SIGNATURE_BASE            0x008E0400 //!< Address where signature bytes start.

#define XNVM_FLASH_PAGE_SIZE			512			//

#define XNVM_CONTROLLER_BASE 0x01C0               //!< NVM Controller register base address.
#define XNVM_CONTROLLER_CMD_REG_OFFSET 0x0A       //!< NVM Controller Command Register offset.
#define XNVM_CONTROLLER_STATUS_REG_OFFSET 0x0F    //!< NVM Controller Status Register offset.
#define XNVM_CONTROLLER_CTRLA_REG_OFFSET 0x0B     //!< NVM Controller Control Register A offset.

#define XNVM_CTRLA_CMDEX (1 << 0)                 //!< CMDEX bit offset.
#define XNVM_NVMEN (1 << 1)                       //!< NVMEN bit offset.
#define XNVM_NVM_BUSY (1 << 7)                    //!< NVMBUSY bit offset.

#define XOCD_STATUS_REGISTER_ADDRESS 0x00         //!< PDI status register address.
#define XOCD_RESET_REGISTER_ADDRESS  0x01         //!< PDI reset register address.
#define XOCD_RESET_SIGNATURE         0x59         //!< PDI reset Signature.
#define XOCD_FCMR_ADDRESS 0x05

#define NVM_PAGE_ORDER    9                       //!< NVM Page Order of 2.
#define NVM_PAGE_SIZE   (1 << NVM_PAGE_ORDER)     //!< NVM Page Size.
#define NVM_EEPROM_PAGE_SIZE 32                   //!< EEPROM Page Size.
#define NVM_LOCKBIT_ADDR  7                       //!< Lockbit address.
#define NVM_MCU_CONTROL   0x90                    //!< MCU Control base address.

#define NVM_COMMAND_BUFFER_SIZE 20                //!< NVM Command buffer size.
#define WAIT_RETRIES_NUM 1000                     //!< Retry Number.
#define DUMMY_BYTE 0x55                           //!< Dummy byte for Dummy writing.

#endif
