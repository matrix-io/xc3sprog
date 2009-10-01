unsigned int detect_chain(Jtag &jtag, DeviceDB &db);
int getIO(std::auto_ptr<IOBase> *io, char const *cable, int subtype, int  vendor, int  product, char const *dev, char const *desc, char const *serial);
