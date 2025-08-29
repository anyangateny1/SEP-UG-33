import numpy as np

class Block:
    def __init__(self, x, y, z, width, height, depth, tag, x_offset=0, y_offset=0, z_offset=0):
        self.x, self.y, self.z = (x, y, z)
        self.x_offset, self.y_offset, self.z_offset = (x_offset, y_offset, z_offset)
        self.width, self.height, self.depth = (width, height, depth)
        self.x_end, self.y_end, self.z_end = (x + width, y + height, z + depth)
        self.tag = tag
        self.is_uniform = False
    
    def set_width(self, width):
        self.width = width
        self.x_end = self.x + width
    
    def set_height(self, height):
        self.height = height
        self.y_end = self.y + height
        
    def set_depth(self, depth):
        self.depth = depth
        self.z_end = self.z + depth
        
    def block_slice(self, model:np.ndarray):
        """
        Slice the model by the block position and size.
        """
        x, y, z = (self.x_offset, self.y_offset, self.z_offset)
        x_end, y_end, z_end = (self.x_offset + self.width, self.y_offset + self.height, self.z_offset + self.depth)
        return model[z:z_end, y:y_end, x:x_end]
        
    def check_is_uniform(self, model: np.ndarray):
        return np.all(self.block_data(model) == self.tag)
    
    def get_mode(self, model: np.ndarray, compressed: np.ndarray):
        """
        Returns the mode of the uncompressed parts of the block.
        """
        tags_slice = self.block_slice(model)
        compressed_slice = self.block_slice(compressed)
        masked = tags_slice[~compressed_slice]
        
        vals, counts = np.unique(masked, return_counts=True)
        return vals[np.argmax(counts)]

    def print_block(self, label):
        print(f"{self.x},{self.y},{self.z},{self.width},{self.height},{self.depth},{label}")
