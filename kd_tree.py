import numpy as np

from block import Block
from slices_section import SlicesSection
        
class KdTree:
    def __init__(self, slices: SlicesSection, tag_table):
        self.slices = slices
        self.tag_table = tag_table
        
    def find_split(self, block: Block):
        best_axis = None
        best_index = 0
        min_centre_dist = max(block.width, block.height, block.depth)
        block.is_uniform = True
        for z in range(block.z, block.z + block.depth):
            for y in range(block.y, block.y + block.height):
                for x in range(block.x, block.x + block.width):
                    current_tag = self.slices.get_block_tag(x, y, z)
                    if (x + 1 < block.x + block.width) and (current_tag != self.slices.get_block_tag(x + 1, y, z)):
                        centre_dist = abs(x - (block.x + (block.width // 2)))
                        if centre_dist < min_centre_dist:
                            best_axis = 'x'
                            best_index = x
                            min_centre_dist = centre_dist
                        
                        block.is_uniform = False
                    if (y + 1 < block.y + block.height) and (current_tag != self.slices.get_block_tag(x, y + 1, z)):
                        centre_dist = abs(y - (block.y + (block.height // 2)))
                        if centre_dist < min_centre_dist:
                            best_axis = 'y'
                            best_index = y
                            min_centre_dist = centre_dist
                        
                        block.is_uniform = False
                    if (z + 1 < block.z + block.depth) and (current_tag != self.slices.get_block_tag(x, y, z + 1)):
                        centre_dist = abs(z - (block.z + (block.depth // 2)))
                        if centre_dist < min_centre_dist:
                            best_axis = 'z'
                            best_index = z
                            min_centre_dist = centre_dist
                            
                        block.is_uniform = False
                        
        return (best_axis, best_index)
    
    def build_kd_tree(self, block):
        split_axis, split_index = self.find_split(block)
        
        if block.is_uniform:
            block.print_block(self.tag_table[block.tag])
            return
        
        match split_axis:
            case 'x':
                # left <= X
                left_x, left_y, left_z = (block.x, block.y, block.z)
                left_w, left_h, left_d = (split_index - block.x + 1, block.height, block.depth)

                # right > X
                right_x, right_y, right_z = (split_index + 1, block.y, block.z)
                right_w, right_h, right_d = (block.x + block.width - (split_index + 1), block.height, block.depth)

            case 'y':
                # left <= Y
                left_x, left_y, left_z = (block.x, block.y, block.z)
                left_w, left_h, left_d = (block.width, split_index - block.y + 1, block.depth)
                
                # right > Y
                right_x, right_y, right_z = (block.x, split_index + 1, block.z)
                right_w, right_h, right_d = (block.width, block.y + block.height - (split_index + 1), block.depth)
                
            case 'z':
                # left <= Z
                left_x, left_y, left_z = (block.x, block.y, block.z)
                left_w, left_h, left_d = (block.width, block.height, split_index - block.z + 1)
                
                # right > Z
                right_x, right_y, right_z = (block.x, block.y, split_index + 1)
                right_w, right_h, right_d = (block.width, block.height, block.z + block.depth - (split_index + 1))
            
        left_tag = self.slices.get_block_tag(left_x, left_y, left_z)
        left_block = Block(left_x, left_y, left_z, left_w, left_h, left_d, left_tag)
        self.build_kd_tree(left_block)
        
        right_tag = self.slices.get_block_tag(right_x, right_y, right_z)
        right_block = Block(right_x, right_y, right_z, right_w, right_h, right_d, right_tag)
        self.build_kd_tree(right_block)
