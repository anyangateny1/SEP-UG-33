import numpy as np

from block import Block
from slices_section import SlicesSection
        
class KdTree:
    def __init__(self, slices: SlicesSection, tag_table):
        self.slices = slices
        self.tag_table = tag_table
    
    def build_kd_tree(self, block, split_axis='x'):
        is_uniform, split_x, split_y, split_z = block.check_is_uniform(self.slices, split_axis)
        if is_uniform:
            block.print_block(self.tag_table[block.tag])
            return
        
        match split_axis:
            case 'x':
                next_split = 'y'
                
                if block.width == 1:
                    self.build_kd_tree(block, next_split)
                    return
                
                if split_x == block.x:
                    split_x = block.x + 1
                    
                # left < X
                left_x, left_y, left_z = (block.x, block.y, block.z)
                left_w, left_h, left_d = (split_x - block.x, block.height, block.depth)
                
                # right >= X
                right_x, right_y, right_z = (split_x, block.y, block.z)
                right_w, right_h, right_d = (block.width - (split_x - block.x), block.height, block.depth)

            case 'y':
                next_split = 'z'
                
                if block.height == 1:
                    self.build_kd_tree(block, next_split)
                    return
                
                if split_y == block.y:
                    split_y = block.y + 1
                
                # left < Y
                left_x, left_y, left_z = (block.x, block.y, block.z)
                left_w, left_h, left_d = (block.width, split_y - block.y, block.depth)
                
                # right >= Y
                right_x, right_y, right_z = (block.x, split_y, block.z)
                right_w, right_h, right_d = (block.width, block.height - (split_y - block.y), block.depth)
                
            case 'z':
                next_split = 'x'
                
                if block.depth == 1:
                    self.build_kd_tree(block, next_split)
                    return
                
                if split_z == block.z:
                    split_z = block.z + 1
                
                # left < Y
                left_x, left_y, left_z = (block.x, block.y, block.z)
                left_w, left_h, left_d = (block.width, block.height, split_z - block.z)
                
                # right >= Y
                right_x, right_y, right_z = (block.x, block.y, split_z)
                right_w, right_h, right_d = (block.width, block.height, block.depth - (split_z - block.z))
            
        left_tag = self.slices.get_block_tag(left_x, left_y, left_z)
        left_block = Block(left_x, left_y, left_z, left_w, left_h, left_d, left_tag)
        self.build_kd_tree(left_block, next_split)
        
        right_tag = self.slices.get_block_tag(right_x, right_y, right_z)
        right_block = Block(right_x, right_y, right_z, right_w, right_h, right_d, right_tag)
        self.build_kd_tree(right_block, next_split)
