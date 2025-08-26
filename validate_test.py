import numpy as np

width, height, depth = (64,16,5)
model = np.empty((height, width, depth), dtype='U1')

label_table = {
    'sea': 'o',
    'WA': 'w',
    'NT': 'n',
    'SA': 's',
    'QLD': 'q',
    'SA': 's',
    'NSW': 'e',
    'VIC': 'v',
    'TAS': 't'
}

coords = set()

line = input()
while(line):
    line = line.split(',')
    x, y, z = (int(line[0]), int(line[1]), int(line[2]))
    block_w, block_h, block_d = (int(line[3]), int(line[4]), int(line[5]))
    tag = label_table[line[6]]
    for dz in range(block_d):
        z_idx = dz - z
        for dy in range(block_h):
            y_idx = dy - y
            for dx in range(block_w):
                x_idx = x + dx
                model[y_idx, x_idx, z_idx] = tag
                
                if ((x_idx, y_idx, z_idx) in coords):
                    print(f"{(x_idx, y_idx, z_idx)} appears twice")
                    
                coords.add((x_idx, y_idx, z_idx))


                
    line = input()


for z in range(depth):
    for y in range(height):
        for x in range(width):
            print(model[y, x, z], end="")
            if model[y, x, z] == '':
                print('-', end="")
        print()
    print()