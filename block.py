from slices_section import SlicesSection

class Block:
    def __init__(self, x, y, z, width, height, depth, tag):
        self.x, self.y, self.z = (x, y, z)
        self.width, self.height, self.depth = (width, height, depth)
        self.tag = tag

    def print_block(self, label):
        print(f"{self.x},{self.y},{self.z},{self.width},{self.height},{self.depth},{label}")
