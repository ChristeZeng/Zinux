for i in range(32):
    print("ld x%d, %d(sp)" % (31 - i, 232 - i*8))
