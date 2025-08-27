import numpy as np

class SlicesSection:
    def __init__(self, width, height, num_slices, top_slice=0):
        self.model = np.empty((num_slices, height, width), dtype = 'U1')
        self.top_slice = top_slice
        self.n = num_slices
    
    def set_block_tag(self, x, y, z, tag):
        self.model[z - self.top_slice, y, x] = tag
    
    def get_block_tag(self, x, y, z):
        return self.model[z - self.top_slice, y, x]