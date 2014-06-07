void SerialInit(void);
void SerialWriteByte(char c);
void SerialWriteString(const char *s);
void SerialWriteRomString(const rom char *s);
unsigned char SerialReadByte(void);
void SerialHandler(void);
unsigned char SerialDataAvailable(void);

