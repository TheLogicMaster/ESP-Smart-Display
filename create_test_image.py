import random
 
f = open('image.bin', 'wb')
byte_arr = [(random.randrange(0, 255) if i > 3 else 0) for i in range(0, 32 * 64 * 2)]

binary_format = bytearray(byte_arr)
f.write(binary_format)
f.close();
