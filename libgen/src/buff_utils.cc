unsigned char getBuffOne(unsigned char *buffer, unsigned buffer_size, unsigned *offset) {
    unsigned off = *offset;
    unsigned char res;
    if (off < buffer_size) {
        res = buffer[off++];
        *offset = off;
    } else {
        res = 0;
    }
    return res;
}

unsigned short getBuffTwo(unsigned char *buffer, unsigned buffer_size, unsigned *offset) {
    return ((unsigned short)getBuffOne(buffer, buffer_size, offset)) | (((unsigned short)getBuffOne(buffer, buffer_size, offset)) << 8);
}

unsigned int getBuffFour(unsigned char *buffer, unsigned buffer_size, unsigned *offset) {
    return ((unsigned int)getBuffTwo(buffer, buffer_size, offset)) | (((unsigned int)getBuffTwo(buffer, buffer_size, offset)) << 16);
    //return ((unsigned int)getBuffOne(buffer, buffer_size, offset)) |
    //    (((unsigned int)getBuffOne(buffer, buffer_size, offset)) << 8) |
    //    (((unsigned int)getBuffOne(buffer, buffer_size, offset)) << 16) |
    //    (((unsigned int)getBuffOne(buffer, buffer_size, offset)) << 24);
}

unsigned int getBuffEight(unsigned char *buffer, unsigned buffer_size, unsigned *offset) {
    return ((unsigned long int)getBuffFour(buffer, buffer_size, offset)) | (((unsigned long int)getBuffFour(buffer, buffer_size, offset)) << 32);
}
