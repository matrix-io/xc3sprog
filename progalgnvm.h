#ifndef PROGALGNVM_H
#define PROGALGNVM_H

#include <stdint.h>

#include <pdioverjtag.h>

#define XNVM_PDI_LDS_INSTR    0x00 //!< LDS instruction.
#define XNVM_PDI_STS_INSTR    0x40 //!< STS instruction.
#define XNVM_PDI_LD_INSTR     0x20 //!< LD instruction.
#define XNVM_PDI_ST_INSTR     0x60 //!< ST instruction.
#define XNVM_PDI_LDCS_INSTR   0x80 //!< LDCS instruction.
#define XNVM_PDI_STCS_INSTR   0xC0 //!< STCS instruction.
#define XNVM_PDI_REPEAT_INSTR 0xA0 //!< REPEAT instruction.
#define XNVM_PDI_KEY_INSTR    0xE0 //!< KEY instruction.

/** Byte size address mask for LDS and STS instruction */
#define XNVM_PDI_BYTE_ADDRESS_MASK 0x00
/** Word size address mask for LDS and STS instruction */
#define XNVM_PDI_WORD_ADDRESS_MASK 0x04
/** 3 bytes size address mask for LDS and STS instruction */
#define XNVM_PDI_3BYTES_ADDRESS_MASK 0x08
/** Long size address mask for LDS and STS instruction */
#define XNVM_PDI_LONG_ADDRESS_MASK 0x0C
/** Byte size data mask for LDS and STS instruction */
#define XNVM_PDI_BYTE_DATA_MASK 0x00
/** Word size data mask for LDS and STS instruction */
#define XNVM_PDI_WORD_DATA_MASK 0x01
/** 3 bytes size data mask for LDS and STS instruction */
#define XNVM_PDI_3BYTES_DATA_MASK 0x02
/** Long size data mask for LDS and STS instruction */
#define XNVM_PDI_LONG_DATA_MASK 0x03
/** Byte size address mask for LDS and STS instruction */
#define XNVM_PDI_LD_PTR_STAR_MASK 0x00
/** Word size address mask for LDS and STS instruction */
#define XNVM_PDI_LD_PTR_STAR_INC_MASK 0x04
/** 3 bytes size address mask for LDS and STS instruction */
#define XNVM_PDI_LD_PTR_ADDRESS_MASK 0x08

#define XNVM_CMD_NOP                         0x00 //!< No Operation.
#define XNVM_CMD_CHIP_ERASE                  0x40 //!< Chip Erase.
#define XNVM_CMD_READ_NVM_PDI                0x43 //!< Read NVM PDI.
#define XNVM_CMD_LOAD_FLASH_PAGE_BUFFER      0x23 //!< Load Flash Page Buffer.
#define XNVM_CMD_ERASE_FLASH_PAGE_BUFFER     0x26 //!< Erase Flash Page Buffer.
#define XNVM_CMD_ERASE_FLASH_PAGE            0x2B //!< Erase Flash Page.
#define XNVM_CMD_WRITE_FLASH_PAGE            0x2E //!< Flash Page Write.
#define XNVM_CMD_ERASE_AND_WRITE_FLASH_PAGE  0x2F //!< Erase & Write Flash Page.
#define XNVM_CMD_CALC_CRC_ON_FLASH           0x78 //!< Flash CRC.

#define XNVM_CMD_ERASE_APP_SECTION           0x20 //!< Erase Application Section.
#define XNVM_CMD_ERASE_APP_PAGE              0x22 //!< Erase Application Section.
#define XNVM_CMD_WRITE_APP_SECTION           0x24 //!< Write Application Section.
#define XNVM_CMD_ERASE_AND_WRITE_APP_SECTION 0x25 //!< Erase & Write Application Section Page.
#define XNVM_CMD_CALC_CRC_APP_SECTION        0x38 //!< Application Section CRC.

#define XNVM_CMD_ERASE_BOOT_SECTION          0x68 //!< Erase Boot Section.
#define XNVM_CMD_ERASE_BOOT_PAGE             0x2A //!< Erase Boot Loader Section Page.
#define XNVM_CMD_WRITE_BOOT_PAGE             0x2C //!< Write Boot Loader Section Page.
#define XNVM_CMD_ERASE_AND_WRITE_BOOT_PAGE   0x2D //!< Erase & Write Boot Loader Section Page.
#define XNVM_CMD_CALC_CRC_BOOT_SECTION       0x39 //!< Boot Loader Section CRC.

#define XNVM_CMD_READ_USER_SIGN              0x03 //!< Read User Signature Row.
#define XNVM_CMD_ERASE_USER_SIGN             0x18 //!< Erase User Signature Row.
#define XNVM_CMD_WRITE_USER_SIGN             0x1A //!< Write User Signature Row.
#define XNVM_CMD_READ_CALIB_ROW              0x02 //!< Read Calibration Row.

#define XNVM_CMD_READ_FUSE                   0x07 //!< Read Fuse.
#define XNVM_CMD_WRITE_FUSE                  0x4C //!< Write Fuse.
#define XNVM_CMD_WRITE_LOCK_BITS             0x08 //!< Write Lock Bits.

#define XNVM_CMD_LOAD_EEPROM_PAGE_BUFFER     0x33 //!< Load EEPROM Page Buffer.
#define XNVM_CMD_ERASE_EEPROM_PAGE_BUFFER    0x36 //!< Erase EEPROM Page Buffer.

#define XNVM_CMD_ERASE_EEPROM                0x30 //!< Erase EEPROM.
#define XNVM_CMD_ERASE_EEPROM_PAGE           0x32 //!< Erase EEPROM Page.
#define XNVM_CMD_WRITE_EEPROM_PAGE           0x34 //!< Write EEPROM Page.
#define XNVM_CMD_ERASE_AND_WRITE_EEPROM      0x35 //!< Erase & Write EEPROM Page.
#define XNVM_CMD_READ_EEPROM                 0x06 //!< Read EEPROM.

/**
 * \brief Key used to enable the NVM interface.
 */
#define NVM_KEY_BYTE0 0xFF
#define NVM_KEY_BYTE1 0x88
#define NVM_KEY_BYTE2 0xD8
#define NVM_KEY_BYTE3 0xCD
#define NVM_KEY_BYTE4 0x45
#define NVM_KEY_BYTE5 0xAB
#define NVM_KEY_BYTE6 0x89
#define NVM_KEY_BYTE7 0x12


class ProgAlgNVM
{
private:
    PDIoverJTAG *prot;
    int initialized;

    enum PDI_STATUS_CODE xnvm_read_pdi_status(uint8_t *status);
    enum PDI_STATUS_CODE xnvm_wait_for_nvmen(uint32_t retries);
    enum PDI_STATUS_CODE xnvm_ctrl_read_reg(uint16_t reg, uint8_t *value);
    enum PDI_STATUS_CODE xnvm_ctrl_write_reg(uint16_t reg, uint8_t value);
    enum PDI_STATUS_CODE xnvm_ctrl_cmd_write(uint8_t cmd_id);
    enum PDI_STATUS_CODE xnvm_ctrl_read_status(uint8_t *value);
    enum PDI_STATUS_CODE xnvm_erase_application_flash_page
	(uint32_t address, uint8_t *dat_buf, uint16_t length);
    enum PDI_STATUS_CODE xnvm_erase_flash_buffer(uint32_t retries);
    enum PDI_STATUS_CODE xnvm_load_flash_page_buffer
	(uint32_t addr, uint8_t *buf, uint16_t len);
    enum PDI_STATUS_CODE xnvm_st_ptr(uint32_t address);
    enum PDI_STATUS_CODE xnvm_st_star_ptr_postinc(uint8_t value);
    enum PDI_STATUS_CODE xnvm_write_repeat(uint32_t count);
    enum PDI_STATUS_CODE xnvm_ctrl_wait_nvmbusy(uint32_t retries);
    enum PDI_STATUS_CODE xnvm_ctrl_cmdex_write(void);
    enum PDI_STATUS_CODE xnvm_erase_eeprom_buffer(uint32_t retries);
    enum PDI_STATUS_CODE xnvm_load_eeprom_page_buffer
    (uint32_t addr, uint8_t *buf, uint16_t len);
 
 
public:
    uint8_t cmd_buffer[20];
    ProgAlgNVM(PDIoverJTAG *protocoll);
    ~ProgAlgNVM(void);
    enum PDI_STATUS_CODE xnvm_init (void);
    enum PDI_STATUS_CODE xnvm_ioread_byte(uint16_t address, uint8_t *value);
    enum PDI_STATUS_CODE xnvm_iowrite_byte(uint16_t address, uint8_t value);
    enum PDI_STATUS_CODE xnvm_chip_erase(void);
    enum PDI_STATUS_CODE xnvm_application_erase(void);
    enum PDI_STATUS_CODE xnvm_boot_erase(void);
    uint16_t xnvm_read_memory(uint32_t address, uint8_t *data, uint32_t length);
    enum PDI_STATUS_CODE xnvm_erase_program_flash_page
	(uint32_t address, uint8_t *dat_buf, uint16_t length);
    enum PDI_STATUS_CODE xnvm_put_dev_in_reset (void);
    enum PDI_STATUS_CODE xnvm_pull_dev_out_of_reset(void);
    enum PDI_STATUS_CODE xnvm_erase_program_eeprom_page
	(uint32_t address, uint8_t *dat_buf, uint16_t length);
    enum PDI_STATUS_CODE xnvm_erase_user_sign(void);
    enum PDI_STATUS_CODE xnvm_erase_program_user_sign
	(uint32_t address, uint8_t *dat_buf, uint16_t length);
    enum PDI_STATUS_CODE xnvm_write_fuse_bit
	(uint32_t address, uint8_t value, uint32_t retries);
};

#endif //PROGALGNVM_H
