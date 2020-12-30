for i in range(0, 15):
    a = []

    for j in range(0, 10000):
        value = i * 10000 + j
        a.append(value & 0xFF)
        a.append((value >> 8) & 0xFF)
        a.append((value >> 16) & 0xFF)
        a.append((value >> 16) & 0xFF)
        a.append((value >> 8) & 0xFF)
        a.append(value & 0xFF)

    f = open("assets/24bps_part{}.bin".format(i), "wb")
    a_as_bytes = bytes(a)
    f.write(a_as_bytes)
