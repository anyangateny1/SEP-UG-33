from slices_section import SlicesSection

class Block:
    def __init__(self, x, y, z, width, height, depth, tag):
        self.x, self.y, self.z = (x, y, z)
        self.width, self.height, self.depth = (width, height, depth)
        self.bottom_y, self.bottom_z = (self.y + (self.height - 1), self.z + (self.depth - 1))
        self.tag = tag
                        
    def check_is_uniform(self, slices: SlicesSection, scan_axis='z'):
        self.is_uniform = True
        match scan_axis:
            case 'x':
                for x in range(self.x, self.x + self.width):
                    for y in range(self.y, self.y + self.height):
                        for z in range(self.z, self.z + self.depth):
                            if slices.get_block_tag(x, y, z) != self.tag:
                                self.is_uniform = False
                                return (False, x, y, z)
            case 'y':
                for y in range(self.y, self.y + self.height):
                    for x in range(self.x, self.x + self.width):
                        for z in range(self.z, self.z + self.depth):
                            if slices.get_block_tag(x, y, z) != self.tag:
                                self.is_uniform = False
                                return (False, x, y, z)
            case 'z':
                for z in range(self.z, self.z + self.depth):
                    for y in range(self.y, self.y + self.height):
                        for x in range(self.x, self.x + self.width):
                            if slices.get_block_tag(x, y, z) != self.tag:
                                self.is_uniform = False
                                return (False, x, y, z)
        return (True, 0, 0, 0)
    
    def print_block(self, label):
        print(f"{self.x},{self.bottom_y},{self.bottom_z},{self.width},{self.height},{self.depth},{label}")
