file_size = 10 * 1024 * 1024 
file_name = "arquivo.bin"

with open(file_name, "wb") as f:
    f.write(bytearray(file_size))

print(f"{file_name} criado com sucesso!")
