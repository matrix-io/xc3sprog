/**
 * \file
 *
 * \brief XMEGA PDI NVM command driver
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

/* Adaptation to xc3sprog 
   Uwe Bonnes bon@elektron.ikp.physik.tu-darmstadt.de 2011
*/
#include <string.h>

#include "progalgnvm.h"
#include "atxmega128a1_nvm_regs.h"

ProgAlgNVM::ProgAlgNVM(PDIoverJTAG *protocoll)
{
    prot = protocoll;
    initialized = 0;
}

ProgAlgNVM::~ProgAlgNVM(void)
{
    prot = 0;
    initialized = 0;
}

/**
 * \brief Initiliazation function for the PDI interface
 *
 * This function initializes the PDI interface agains
 * the connected target device.
 *
 * \retval STATUS_OK init ok
 * \retval ERR_TIMEOUT the init timed out
 */
enum PDI_STATUS_CODE ProgAlgNVM::xnvm_init(void)
{
    PDI_STATUS_CODE retval;
    if(initialized == 0){

	/* Put the device in reset mode */
	xnvm_put_dev_in_reset();

	/* Create the key command */
	cmd_buffer[0] = XNVM_PDI_KEY_INSTR;
	cmd_buffer[1] = NVM_KEY_BYTE0;
	cmd_buffer[2] = NVM_KEY_BYTE1;
	cmd_buffer[3] = NVM_KEY_BYTE2;
	cmd_buffer[4] = NVM_KEY_BYTE3;
	cmd_buffer[5] = NVM_KEY_BYTE4;
	cmd_buffer[6] = NVM_KEY_BYTE5;
	cmd_buffer[7] = NVM_KEY_BYTE6;
	cmd_buffer[8] = NVM_KEY_BYTE7;

	prot->pdi_write(cmd_buffer, 9);

	retval = xnvm_ctrl_wait_nvmbusy(WAIT_RETRIES_NUM);

	initialized = 1;
    }

    return retval;
}

/**
 * \brief Function for putting the device into reset
 *
 * \retval STATUS_OK if all went well
 * \retval ERR_IO_ERROR if the pdi write failed
 */
enum PDI_STATUS_CODE ProgAlgNVM::xnvm_put_dev_in_reset (void)
{
    /* Reset the device */
    cmd_buffer[0] = XNVM_PDI_STCS_INSTR | XOCD_RESET_REGISTER_ADDRESS;
    cmd_buffer[1] = XOCD_RESET_SIGNATURE;
    if(prot->pdi_write(cmd_buffer, 2)){
	return ERR_IO_ERROR;
    }
    return STATUS_OK;
}

/**
 * \brief Function for releasing the reset of the device
 *
 * \retval STATUS_OK if all went well
 * \retval ERR_IO_ERROR if the pdi write failed
 */
enum PDI_STATUS_CODE ProgAlgNVM::xnvm_pull_dev_out_of_reset (void)
{

    /* Pull device out of reset */
    cmd_buffer[0] = XNVM_PDI_STCS_INSTR | XOCD_RESET_REGISTER_ADDRESS;
    cmd_buffer[1] = 0;
    if(prot->pdi_write(cmd_buffer, 2)){
	return ERR_IO_ERROR;
    }
    return STATUS_OK;
}

/**
 *  \internal
 *  \brief Wait until the NVM module has completed initialization
 *
 *  \param  retries the retry count.
 *  \retval STATUS_OK the NVMEN was set successfully.
 *  \retval ERR_BAD_DATA One of the bytes sent was corrupted during transmission.
 *  \retval ERR_TIMEOUT Time out.
 */
enum PDI_STATUS_CODE ProgAlgNVM::xnvm_wait_for_nvmen(uint32_t retries)
{
    uint8_t pdi_status;

    while (retries != 0) {
	if (xnvm_read_pdi_status(&pdi_status) != STATUS_OK) {
	    return ERR_BAD_DATA;
	}
	if ((pdi_status & XNVM_NVMEN) != 0) {
	    return STATUS_OK;
	}
	--retries;
    }
    return ERR_TIMEOUT;

}

/**
 *  \internal
 *  \brief Read the PDI Controller's STATUS register
 *
 *  \param  status the status buffer pointer.
 *  \retval STATUS_OK read successfully.
 *  \retval ERR_BAD_DATA One of the bytes sent was corrupted during transmission.
 *  \retval ERR_TIMEOUT Time out.
 */
enum PDI_STATUS_CODE ProgAlgNVM::xnvm_read_pdi_status(uint8_t *status)
{
    enum PDI_STATUS_CODE ret = STATUS_OK;

    cmd_buffer[0] = XNVM_PDI_LDCS_INSTR;
    cmd_buffer[1] = 0x00;
    if (STATUS_OK != prot->pdi_write(cmd_buffer, 2)) {
	ret = ERR_BAD_DATA;
    }
    if (prot->pdi_read(status, 1, WAIT_RETRIES_NUM) == 0) {
	ret = ERR_TIMEOUT;
    }

    return ret;
}

/**
 *  \brief Read the IO space register with NVM controller
 *
 *  \param  address the register address in the IO space.
 *  \param  value the value buffer pointer.
 *  \retval STATUS_OK read successfully.
 *  \retval ERR_BAD_DATA One of the bytes sent was corrupted during transmission.
 *  \retval ERR_TIMEOUT Time out.
 */
enum PDI_STATUS_CODE ProgAlgNVM::xnvm_ioread_byte(uint16_t address, uint8_t *value)
{
    enum PDI_STATUS_CODE ret = STATUS_OK;
    uint32_t register_address;
    int res;

    cmd_buffer[0] = XNVM_PDI_LDS_INSTR | XNVM_PDI_LONG_ADDRESS_MASK |
	XNVM_PDI_BYTE_DATA_MASK;

    register_address = XNVM_DATA_BASE + address;

    memmove((cmd_buffer + 1), (uint8_t*)&register_address, 4);

    ret = prot->pdi_write(cmd_buffer, 5);
    if( ret == STATUS_OK)
    {
	res = prot->pdi_read(value, 1, WAIT_RETRIES_NUM);
	if (res != 0)
	    ret = ERR_BAD_DATA;
    }
    return ret;
}

/**
 *  \brief Write the IO space register with NVM controller
 *
 *  \param  address the register address in the IO space.
 *  \param  value the value which should be write into the address.
 *  \retval STATUS_OK write successfully.
 *  \retval ERR_BAD_DATA One of the bytes sent was corrupted during transmission.
 *  \retval ERR_TIMEOUT Time out.
 */
enum PDI_STATUS_CODE ProgAlgNVM::xnvm_iowrite_byte(uint16_t address, uint8_t value)
{

    uint32_t register_address = XNVM_DATA_BASE + address;

    cmd_buffer[0] = XNVM_PDI_STS_INSTR | XNVM_PDI_LONG_ADDRESS_MASK |
	XNVM_PDI_BYTE_DATA_MASK;

    memmove((cmd_buffer + 1), (uint8_t*)&register_address, 4);
    cmd_buffer[5] = value;

    return (prot->pdi_write(cmd_buffer, 6));
}

/**
 *  \internal
 *  \brief Read the NVM Controller's status register
 *
 *  \param  value the NVM Controller's status buffer pointer.
 *  \retval STATUS_OK read successfully.
 *  \retval ERR_TIMEOUT Time out.
 */
enum PDI_STATUS_CODE ProgAlgNVM::xnvm_ctrl_read_status(uint8_t *value)
{
    return xnvm_ctrl_read_reg(XNVM_CONTROLLER_STATUS_REG_OFFSET, value);
}

/**
 *  \internal
 *  \brief Read the NVM Controller's register
 *
 *  \param  reg the offset of the NVM Controller register.
 *  \param  value the pointer of the value buffer.
 *  \retval STATUS_OK read succussfully.
 *  \retval ERR_TIMEOUT Time out.
 */
enum PDI_STATUS_CODE ProgAlgNVM::xnvm_ctrl_read_reg(uint16_t reg, uint8_t *value)
{
    uint16_t address;

    address = XNVM_CONTROLLER_BASE + reg;
    return xnvm_ioread_byte(address, value);
}

/**
 *  \internal
 *  \brief Write the NVM Controller's register
 *
 *  \param  reg the offset of the NVM Controller register.
 *  \param  value the value which should be write into the register.
 *  \retval STATUS_OK write succussfully.
 *  \retval ERR_BAD_DATA One of the bytes sent was corrupted during transmission.
 *  \retval ERR_TIMEOUT Time out.
 */
enum PDI_STATUS_CODE ProgAlgNVM::xnvm_ctrl_write_reg(uint16_t reg, uint8_t value)
{
    uint16_t address;

    address = XNVM_CONTROLLER_BASE + reg;
    return xnvm_iowrite_byte(address, value);
}

/**
 * \internal
 * \brief Write the NVM CTRLA register CMDEX
 *
 * \retval STATUS_OK write successful.
 * \retval STATUS_OK write successfully.
 * \retval ERR_BAD_DATA One of the bytes sent was corrupted during transmission.
 * \retval ERR_TIMEOUT Time out.
 */
enum PDI_STATUS_CODE ProgAlgNVM::xnvm_ctrl_cmdex_write(void)
{
    return xnvm_ctrl_write_reg(XNVM_CONTROLLER_CTRLA_REG_OFFSET, XNVM_CTRLA_CMDEX);
}

/**
 *  \internal
 *  \brief Write NVM command register
 *
 *  \param  cmd_id the command code which should be write into the NVM command register.
 *  \retval STATUS_OK write successfully.
 *  \retval ERR_BAD_DATA One of the bytes sent was corrupted during transmission.
 *  \retval ERR_TIMEOUT Time out.
 */
enum PDI_STATUS_CODE ProgAlgNVM::xnvm_ctrl_cmd_write(uint8_t cmd_id)
{
    return xnvm_ctrl_write_reg(XNVM_CONTROLLER_CMD_REG_OFFSET, cmd_id);
}

/**
 *  \brief Erase the whole chip
 *
 *  \retval STATUS_OK erase chip succussfully.
 *  \retval ERR_TIMEOUT Time out.
 */
enum PDI_STATUS_CODE ProgAlgNVM::xnvm_chip_erase(void)
{
    /* Write the chip erase command to the NVM command reg */
    xnvm_ctrl_cmd_write(XNVM_CMD_CHIP_ERASE);
    /* Write the CMDEX to execute command */
    xnvm_ctrl_cmdex_write();
    return xnvm_wait_for_nvmen(WAIT_RETRIES_NUM);
}

/**
 *  \brief Erase the application section chip
 *
 *  \retval STATUS_OK erase chip succussfully.
 *  \retval ERR_TIMEOUT Time out.
 */
enum PDI_STATUS_CODE ProgAlgNVM::xnvm_application_erase(void)
{
    /* Write the chip erase command to the NVM command reg */
    xnvm_ctrl_cmd_write(XNVM_CMD_ERASE_APP_SECTION);
    /* Write the CMDEX to execute command */
    xnvm_ctrl_cmdex_write();
    return xnvm_wait_for_nvmen(WAIT_RETRIES_NUM);
}

/**
 *  \brief Erase the boot section chip
 *
 *  \retval STATUS_OK erase chip succussfully.
 *  \retval ERR_TIMEOUT Time out.
 */
enum PDI_STATUS_CODE ProgAlgNVM::xnvm_boot_erase(void)
{
    /* Write the chip erase command to the NVM command reg */
    xnvm_ctrl_cmd_write(XNVM_CMD_ERASE_BOOT_SECTION);
    /* Write the CMDEX to execute command */
    xnvm_ctrl_cmdex_write();
    return xnvm_wait_for_nvmen(WAIT_RETRIES_NUM);
}

/**
 *  \internal
 *  \brief Load the flash page buffer
 *
 *  \param  addr the flash address.
 *  \param  buf the pointer which points to the data buffer.
 *  \param  len the length of data.
 *  \retval STATUS_OK write succussfully.
 *  \retval ERR_BAD_DATA One of the bytes sent was corrupted during transmission.
 *  \retval ERR_INVALID_ARG Invalid argument.
 */
enum PDI_STATUS_CODE ProgAlgNVM::xnvm_load_flash_page_buffer(uint32_t addr, uint8_t *buf, uint16_t len)
{
    if (buf == NULL || len == 0) {
	return ERR_INVALID_ARG;
    }

    xnvm_ctrl_cmd_write(XNVM_CMD_LOAD_FLASH_PAGE_BUFFER);
    xnvm_st_ptr(addr);

    if (len > 1) {
	xnvm_write_repeat(len);
    } else {
	return xnvm_st_star_ptr_postinc(*buf);
    }

    cmd_buffer[0] = XNVM_PDI_ST_INSTR | XNVM_PDI_LD_PTR_STAR_INC_MASK |
	XNVM_PDI_BYTE_DATA_MASK;
    prot->pdi_write(cmd_buffer, 1);

    return prot->pdi_write(buf, len);
}

/**
 *  \internal
 *  \brief Erase the flash buffer with NVM controller.
 *
 *  \param  retries the time out delay number.
 *  \retval STATUS_OK erase successfully.
 *  \retval ERR_TIMEOUT Time out.
 */
enum PDI_STATUS_CODE ProgAlgNVM::xnvm_erase_flash_buffer(uint32_t retries)
{
    xnvm_st_ptr(0);
    xnvm_ctrl_cmd_write(XNVM_CMD_ERASE_FLASH_PAGE_BUFFER);
    xnvm_ctrl_cmdex_write();

    return xnvm_ctrl_wait_nvmbusy(retries);
}

/**
 *  \internal
 *  \brief Erase and program the flash page buffer with NVM controller.
 *
 *  \param  address the address of the flash.
 *  \param  dat_buf the pointer which points to the data buffer.
 *  \param  length the data length.
 *  \retval STATUS_OK program succussfully.
 *  \retval ERR_TIMEOUT Time out.
 */
enum PDI_STATUS_CODE ProgAlgNVM::xnvm_erase_program_flash_page(uint32_t address, uint8_t *dat_buf, uint16_t length)
{
    address = address + XNVM_FLASH_BASE;

    xnvm_erase_flash_buffer(WAIT_RETRIES_NUM);
    xnvm_load_flash_page_buffer(address, dat_buf, length);
    xnvm_ctrl_cmd_write(XNVM_CMD_ERASE_AND_WRITE_APP_SECTION);

    /* Dummy write for starting the erase and write command */
    xnvm_st_ptr(address);
    xnvm_st_star_ptr_postinc(DUMMY_BYTE);

    return xnvm_ctrl_wait_nvmbusy(WAIT_RETRIES_NUM);
}

/**
 *  \internal
 *  \brief Erase and program the flash page buffer with NVM controller.
 *
 *  \param  address the address of the flash.
 *  \param  dat_buf the pointer which points to the data buffer.
 *  \param  length the data length.
 *  \retval STATUS_OK program succussfully.
 *  \retval ERR_TIMEOUT Time out.
 */
enum PDI_STATUS_CODE ProgAlgNVM::xnvm_erase_application_flash_page(uint32_t address, uint8_t *dat_buf, uint16_t length)
{
    address = address + XNVM_FLASH_BASE;

    xnvm_erase_flash_buffer(WAIT_RETRIES_NUM);
    xnvm_load_flash_page_buffer(address, dat_buf, length);
    xnvm_ctrl_cmd_write(XNVM_CMD_ERASE_AND_WRITE_APP_SECTION);

    /* Dummy write for starting the erase and write command */
    xnvm_st_ptr(address);
    xnvm_st_star_ptr_postinc(DUMMY_BYTE);

    return xnvm_ctrl_wait_nvmbusy(WAIT_RETRIES_NUM);
}

/**
 *  \internal
 *  \brief Write the repeating number with PDI port
 *
 *  \param  count the repeating number.
 *  \retval STATUS_OK write succussfully.
 *  \retval ERR_BAD_DATA One of the bytes sent was corrupted during transmission.
 */
enum PDI_STATUS_CODE ProgAlgNVM::xnvm_write_repeat(uint32_t count)
{
    uint8_t cmd_len;

    --count;

    if (count < (1 << 8)) {
	cmd_buffer[0] = XNVM_PDI_REPEAT_INSTR | XNVM_PDI_BYTE_DATA_MASK;
	cmd_buffer[1] = count;
	cmd_len = 2;
    } else if (count < ((uint32_t)(1) << 16)) {
	cmd_buffer[0] = XNVM_PDI_REPEAT_INSTR | XNVM_PDI_WORD_DATA_MASK;
	memmove((cmd_buffer + 1), (uint8_t*)&count, 2);
	cmd_len = 3;
    } else if (count < ((uint32_t)(1) << 24)) {
	cmd_buffer[0] = XNVM_PDI_REPEAT_INSTR | XNVM_PDI_3BYTES_DATA_MASK;
	memmove((cmd_buffer + 1), (uint8_t*)&count, 3);
	cmd_len = 4;
    } else {
	cmd_buffer[0] = XNVM_PDI_REPEAT_INSTR | XNVM_PDI_LONG_DATA_MASK;
	memmove((cmd_buffer + 1), (uint8_t*)&count, 4);
	cmd_len = 5;
    }

    return prot->pdi_write(cmd_buffer, cmd_len);
}

/**
 *  \internal
 *  \brief Write a value to a address with *(ptr++) instruction through the PDI Controller.
 *
 *  \param  value the value should be write into the *ptr.
 *  \retval STATUS_OK write succussfully.
 *  \retval ERR_BAD_DATA One of the bytes sent was corrupted during transmission.
 */
enum PDI_STATUS_CODE ProgAlgNVM::xnvm_st_star_ptr_postinc(uint8_t value)
{
    cmd_buffer[0] = XNVM_PDI_ST_INSTR | XNVM_PDI_LD_PTR_STAR_INC_MASK |
	XNVM_PDI_BYTE_DATA_MASK;
    cmd_buffer[1] = value;

    return prot->pdi_write(cmd_buffer, 2);
}

/**
 *  \internal
 *  \brief Write a address in PDI Controller's pointer.
 *
 *  \param  address the address which should be written into the ptr.
 *  \retval STATUS_OK write successfully.
 *  \retval ERR_BAD_DATA One of the bytes sent was corrupted during transmission.
 */
enum PDI_STATUS_CODE ProgAlgNVM::xnvm_st_ptr(uint32_t address)
{
    cmd_buffer[0] = XNVM_PDI_ST_INSTR | XNVM_PDI_LD_PTR_ADDRESS_MASK |
	XNVM_PDI_LONG_DATA_MASK;

    memmove((cmd_buffer + 1), (uint8_t*)&address, 4);

    return prot->pdi_write(cmd_buffer, 5);
}

/**
 *  \brief Read the memory (include flash, eeprom, user signature, fuse bits)with NVM controller.
 *
 *  \param  address the address of the memory.
 *  \param  data the pointer which points to the data buffer.
 *  \param  length the data length.
 *  \retval non-zero the read byte length.
 *  \retval zero read fail.
 */
uint16_t ProgAlgNVM::xnvm_read_memory
(uint32_t address, uint8_t *data, uint32_t length)
{
    xnvm_ctrl_cmd_write(XNVM_CMD_READ_NVM_PDI);
    xnvm_st_ptr(address);

    if (length > 1) {
	xnvm_write_repeat(length);
    }

    cmd_buffer[0] = XNVM_PDI_LD_INSTR | XNVM_PDI_LD_PTR_STAR_INC_MASK |
	XNVM_PDI_BYTE_DATA_MASK;
    prot->pdi_write(cmd_buffer, 1);

    return prot->pdi_read(data, length, WAIT_RETRIES_NUM);
}

/**
 *  \internal
 *  \brief Erase and program the eeprom page buffer with NVM controller.
 *
 *  \param  address the address of the eeprom.
 *  \param  dat_buf the pointer which points to the data buffer.
 *  \param  length the data length.
 *  \retval STATUS_OK program succussfully.
 *  \retval ERR_TIMEOUT Time out.
 */
enum PDI_STATUS_CODE ProgAlgNVM::xnvm_erase_program_eeprom_page(uint32_t address, uint8_t *dat_buf, uint16_t length)
{
    address = address + XNVM_EEPROM_BASE;

    xnvm_erase_eeprom_buffer(WAIT_RETRIES_NUM);
    xnvm_load_eeprom_page_buffer(address, dat_buf, length);
    xnvm_ctrl_cmd_write(XNVM_CMD_ERASE_AND_WRITE_EEPROM);

    /* Dummy write for starting the erase and write command */
    xnvm_st_ptr(address);
    xnvm_st_star_ptr_postinc(DUMMY_BYTE);

    return xnvm_ctrl_wait_nvmbusy(WAIT_RETRIES_NUM);
}

/**
 *  \internal
 *  \brief Erase the eeprom buffer with NVM controller.
 *
 *  \param  retries the time out delay number.
 *  \retval STATUS_OK erase succussfully.
 *  \retval ERR_TIMEOUT Time out.
 */
enum PDI_STATUS_CODE ProgAlgNVM::xnvm_erase_eeprom_buffer(uint32_t retries)
{
    xnvm_st_ptr(0);
    xnvm_ctrl_cmd_write(XNVM_CMD_ERASE_EEPROM_PAGE_BUFFER);

    /* Execute command by setting CMDEX */
    xnvm_ctrl_cmdex_write();

    return xnvm_ctrl_wait_nvmbusy(retries);
}

/**
 *  \internal
 *  \brief Load the eeprom page buffer
 *
 *  \param  addr the eeprom address.
 *  \param  buf the pointer which points to the data buffer.
 *  \param  len the length of data.
 *  \retval STATUS_OK load succussfully.
 *  \retval ERR_BAD_DATA One of the bytes sent was corrupted during transmission.
 *  \retval ERR_INVALID_ARG Invalid argument.
 */
enum PDI_STATUS_CODE ProgAlgNVM::xnvm_load_eeprom_page_buffer(uint32_t addr, uint8_t *buf, uint16_t len)
{
    if (buf == NULL || len == 0) {
	return ERR_INVALID_ARG;
    }

    xnvm_ctrl_cmd_write(XNVM_CMD_LOAD_EEPROM_PAGE_BUFFER);
    xnvm_st_ptr(addr);

    if (len > 1) {
	xnvm_write_repeat(len);
    } else {
	xnvm_st_star_ptr_postinc(*buf);
	return STATUS_OK;
    }

    cmd_buffer[0] = XNVM_PDI_ST_INSTR | XNVM_PDI_LD_PTR_STAR_INC_MASK |
	XNVM_PDI_BYTE_DATA_MASK;
    prot->pdi_write(cmd_buffer, 1);

    return prot->pdi_write(buf, len);
}

/**
 *  \internal
 *  \brief Erase the user signature with NVM controller.
 *
 *  \retval STATUS_OK erase succussfully.
 *  \retval ERR_TIMEOUT time out.
 */
enum PDI_STATUS_CODE ProgAlgNVM::xnvm_erase_user_sign(void)
{
    xnvm_ctrl_cmd_write(XNVM_CMD_ERASE_USER_SIGN);

    /* Dummy write for starting the erase command */
    xnvm_st_ptr(XNVM_SIGNATURE_BASE);
    xnvm_st_star_ptr_postinc(DUMMY_BYTE);

    return xnvm_ctrl_wait_nvmbusy(WAIT_RETRIES_NUM);
}

/**
 *  \internal
 *  \brief Erase and program the user signature with NVM controller.
 *
 *  \param  address the address of the user signature.
 *  \param  dat_buf the pointer which points to the data buffer.
 *  \param  length the data length.
 *  \retval STATUS_OK program succussfully.
 *  \retval ERR_TIMEOUT time out.
 */
enum PDI_STATUS_CODE ProgAlgNVM::xnvm_erase_program_user_sign
(uint32_t address, uint8_t *dat_buf, uint16_t length)
{
    address = address + XNVM_SIGNATURE_BASE;

    xnvm_erase_flash_buffer(WAIT_RETRIES_NUM);
    xnvm_load_flash_page_buffer(address, dat_buf, length);
    xnvm_erase_user_sign();
    xnvm_ctrl_cmd_write(XNVM_CMD_WRITE_USER_SIGN);

    /* Dummy write for starting the write command. */
    xnvm_st_ptr(address);
    xnvm_st_star_ptr_postinc(DUMMY_BYTE);

    return xnvm_ctrl_wait_nvmbusy(WAIT_RETRIES_NUM);
}

/**
 *  \brief Write the fuse bit with NVM controller
 *
 *  \param  address the fuse bit address.
 *  \param  value which should be write into the fuse bit.
 *  \param  retries the time out delay number.
 *  \retval STATUS_OK write succussfully.
 *  \retval ERR_TIMEOUT time out.
 */
enum PDI_STATUS_CODE ProgAlgNVM::xnvm_write_fuse_bit(uint32_t address, uint8_t value, uint32_t retries)
{
    uint32_t register_address;

    xnvm_ctrl_cmd_write(XNVM_CMD_WRITE_FUSE);

    cmd_buffer[0] = XNVM_PDI_STS_INSTR | XNVM_PDI_LONG_ADDRESS_MASK |
	XNVM_PDI_BYTE_DATA_MASK;

    register_address = XNVM_FUSE_BASE + address;

    memmove((cmd_buffer + 1), (uint8_t*)&register_address, 4);
    cmd_buffer[5] = value;

    prot->pdi_write(cmd_buffer, 6);

    return xnvm_ctrl_wait_nvmbusy(retries);
}

/**
 *  \internal
 *  \brief Wait until the NVM Controller is ready.
 *
 *  \param  retries the retry count.
 *  \retval STATUS_OK BUSY bit was set.
 *  \retval ERR_TIMEOUT Time out.
 */
enum PDI_STATUS_CODE ProgAlgNVM::xnvm_ctrl_wait_nvmbusy(uint32_t retries)
{
    uint8_t status;

    while (retries != 0) {
	xnvm_ctrl_read_status(&status);

	/* Check if the NVMBUSY bit is clear in the NVM_STATUS register. */
	if ((status & XNVM_NVM_BUSY) == 0) {
	    return STATUS_OK;
	}
	--retries;
    }
    return ERR_TIMEOUT;
}
