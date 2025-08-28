import numpy as np
from typing import Dict

from block import Block

class BlockGrowth:
    def __init__(self, model_slices: np.ndarray, tag_table: Dict[str, str]):
        self.model = model_slices
        self.tag_table = tag_table
        
    def fit_block(self, mode, width, height, depth):
        """
        Fit a block filled with the mode of the uncomrpessed tags to the
        parent block area.
        If the block does not fit anywhere, recursively decrease its size until
        a block fits.
        Once a block has been fitted, call grow_block function to grow the block.
        """
        for z in range(self.parent_block.z, self.parent_block.z_end):
            z_offset = z - self.parent_block.z
            z_end = z_offset + depth
            if z_end > self.parent_z_end:
                break
            for y in range(self.parent_block.y, self.parent_block.y_end):
                y_offset = y - self.parent_block.y
                y_end = y_offset + height
                if y_end > self.parent_y_end:
                    break
                for x in range(self.parent_block.x, self.parent_block.x_end):
                    x_offset = x - self.parent_block.x
                    x_end = x_offset + width
                    if x_end > self.parent_x_end:
                        break
                    
                    block_window = self.model[z_offset:z_end, y_offset:y_end, x_offset:x_end]
                    compressed_window = self.compressed[z_offset:z_end, y_offset:y_end, x_offset:x_end]
                    if np.all(block_window == mode) and np.all(compressed_window == False):
                        tag = self.model[z_offset, y_offset, x_offset]
                        block = Block(x, y, z, width, height, depth, mode, x_offset, y_offset, z_offset)
                        self.compressed[z_offset:z_end, y_offset:y_end, x_offset:x_end] = True
                        self.grow_block(block)
                        return block
    
        return self.fit_block(mode, width - 1, height - 1, depth - 1)
    
    def grow_block(self, block: Block):
        """
        Grow the block to its maximum size while staying uniform.
        """
        x, y, z = (block.x_offset, block.y_offset, block.z_offset)
        x_end, y_end, z_end = (x + block.width, y + block.height, z + block.depth)
        
        grew = True
        while grew:
            grew = False
            
            # Grow on X axis
            if x_end < self.parent_x_end:
                growth_slice = self.model[z:z_end, y:y_end, x_end]
                compressed_slice = self.compressed[z:z_end, y:y_end, x_end]
                
                if np.all(growth_slice == block.tag) and np.all(compressed_slice == False):
                    block.set_width(block.width + 1)
                    self.compressed[z:z_end, y:y_end, x_end] = True
                    
                    x_end += 1
                    grew = True

            # Grow on Y axis
            if y_end < self.parent_y_end:
                growth_slice = self.model[z:z_end, y_end, x:x_end]
                compressed_slice = self.compressed[z:z_end, y_end, x:x_end]
                if np.all(growth_slice == block.tag) and np.all(compressed_slice == False):
                    block.set_height(block.height + 1)
                    self.compressed[z:z_end, y_end, x:x_end] = True
                    
                    y_end += 1
                    grew = True

            # Grow on Z axis
            if z_end < self.parent_z_end:
                growth_slice = self.model[z_end, y:y_end, x:x_end]
                compressed_slice = self.compressed[z_end, y:y_end, x:x_end]
                if np.all(growth_slice == block.tag) and np.all(compressed_slice == False):
                    block.set_depth(block.depth + 1)
                    self.compressed[z_end, y:y_end, x:x_end] = True
                    
                    z_end += 1
                    grew = True
        
    def run(self, parent_block: Block):
        """
        Run the block growth algorithm.
        Output: print compressed blocks.
        """
        self.parent_block = parent_block
        self.parent_x_end = self.parent_block.x_offset + self.parent_block.width
        self.parent_y_end = self.parent_block.y_offset + self.parent_block.height
        self.parent_z_end = self.parent_block.z_offset + self.parent_block.depth
        
        # Loop until parent block is compressed
        self.compressed = np.zeros((parent_block.depth, parent_block.height, parent_block.width), dtype=bool)
        while not np.all(self.compressed):
            mode = parent_block.get_mode(self.model, self.compressed)
            cube_size = min(self.parent_block.width, self.parent_block.height, self.parent_block.depth)
            block = self.fit_block(mode, cube_size, cube_size, cube_size)
            block.print_block(self.tag_table[block.tag])
