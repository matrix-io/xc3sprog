#ifndef PDIBASE_H
#define PDIBASE_H

enum PDI_STATUS_CODE
{
    STATUS_OK               =     0, //!< Success
    ERR_IO_ERROR            =    -1, //!< I/O error
    ERR_FLUSHED             =    -2, //!< Request flushed from queue
    ERR_TIMEOUT             =    -3, //!< Operation timed out
    ERR_BAD_DATA            =    -4, //!< Data integrity check failed
    ERR_PROTOCOL            =    -5, //!< Protocol error
    ERR_UNSUPPORTED_DEV     =    -6, //!< Unsupported device
    ERR_NO_MEMORY           =    -7, //!< Insufficient memory
    ERR_INVALID_ARG         =    -8, //!< Invalid argument
    ERR_BAD_ADDRESS         =    -9, //!< Bad address
    ERR_BUSY                =   -10, //!< Resource is busy
    ERR_BAD_FORMAT          =   -11, //!< Data format not recognized
};

class PDIBase
{
protected:
    virtual enum PDI_STATUS_CODE pdi_write(const uint8_t *data, uint16_t length) =0;
    virtual uint32_t pdi_read(uint8_t *data, uint32_t length, uint32_t retries) = 0;
};
#endif //PDIBASE_H
