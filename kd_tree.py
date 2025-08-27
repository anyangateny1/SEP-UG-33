import numpy as np
from typing import List
from block import Block
from node import Node
from slices_section import SlicesSection
        
class KdTree:
    def __init__(self, slices: SlicesSection, tag_table, parent_x, parent_y, parent_z):
        self.slices = slices
        self.tag_table = tag_table
        
        self.parent_x, self.parent_y, self.parent_z = parent_x, parent_y, parent_z
    
    def get_leaves(self, node: Node):
        if node is None:
            return []
        if node.left is None and node.right is None and node.is_uniform:
            return [node]
        return self.get_leaves(node.left) + self.get_leaves(node.right)
    
    def are_neighbours(self, node1: Node, node2: Node):
        # Check if neighbours along X
        if (
            node1.tag == node2.tag and
            node1.y == node2.y and
            node1.height == node2.height and
            node1.z == node2.z and
            node1.depth == node2.depth and
            (node1.x + node1.width == node2.x or node2.x + node2.width == node1.x)
        ):
            neighbours = True
            axis = 'x'
            return (neighbours, axis)
        
        # Check if neighbours along Y
        if (
            node1.tag == node2.tag and
            node1.x == node2.x and
            node1.width == node2.width and
            node1.z == node2.z and
            node1.depth == node2.depth and
            (node1.y + node1.height == node2.y or node2.y + node2.height == node1.y)
        ):
            neighbours = True
            axis = 'y'
            return (neighbours, axis)
        
        # Check if neighbours along Z
        if (
            node1.tag == node2.tag and
            node1.x == node2.x and
            node1.width == node2.width and
            node1.y == node2.y and
            node1.height == node2.height and
            (node1.z + node1.depth == node2.z or node2.z + node2.depth == node1.z)
        ):
            neighbours = True
            axis = 'z'
            return (neighbours, axis)
        
        return (False, None)
    
    def remove_node(self, node: Node):
        if node.parent.left is node:
            node.parent.left = None
        elif node.parent.right is node:
            node.parent.right = None
    
    def merge_blocks(self, leaves: List[Node], i, j, root: Node):
        neighbours, axis = self.are_neighbours(leaves[i], leaves[j])
        if neighbours:
            # Perform merge
            match axis:
                case 'x':
                    node1, node2 = (None, None)
                    if leaves[i].x < leaves[j].x:
                        node1, node2 = (leaves[i], leaves[j])
                    else:
                        node1, node2 = (leaves[j], leaves[i])
                        
                    node1.width += node2.width
                    self.remove_node(node2)
                case 'y':
                    node1, node2 = (None, None)
                    if leaves[i].y < leaves[j].y:
                        node1, node2 = (leaves[i], leaves[j])
                    else:
                        node1, node2 = (leaves[j], leaves[i])

                    node1.height += node2.height
                    self.remove_node(node2)
                case 'z':
                    node1, node2 = (None, None)
                    if leaves[i].z < leaves[j].z:
                        node1, node2 = (leaves[i], leaves[j])
                    else:
                        node1, node2 = (leaves[j], leaves[i])
                        
                    node1.depth += node2.depth
                    self.remove_node(node2)
                    
            return True
        
        return False
    
    def post_compression_pass(self, root: Node):
        """
        Merge neighbouring blocks that align with each other to further
        compress.
        """
        merged = True
        while merged:            
            merged = False
            leaves = self.get_leaves(root)
            
            for i in range(len(leaves) - 1):
                for j in range(i + 1, len(leaves)):
                    if self.merge_blocks(leaves, i, j, root):
                        merged = True
                        break
                if merged:
                    break
        
        leaves = self.get_leaves(root)
        for leaf in leaves:
            leaf.print_block(self.tag_table[leaf.tag])
                    
        
    def find_split(self, node: Node):
        """
        Find the best axis and index to split the block.
        """
        best_axis = None
        best_index = 0
        min_centre_dist = max(node.width, node.height, node.depth)
        node.is_uniform = True
        for z in range(node.z, node.z + node.depth):
            for y in range(node.y, node.y + node.height):
                for x in range(node.x, node.x + node.width):
                    current_tag = self.slices.get_block_tag(x, y, z)
                    if (x + 1 < node.x + node.width) and (current_tag != self.slices.get_block_tag(x + 1, y, z)):
                        centre_dist = abs(x - (node.x + (node.width // 2)))
                        if centre_dist < min_centre_dist:
                            best_axis = 'x'
                            best_index = x
                            min_centre_dist = centre_dist
                        
                        node.is_uniform = False
                    if (y + 1 < node.y + node.height) and (current_tag != self.slices.get_block_tag(x, y + 1, z)):
                        centre_dist = abs(y - (node.y + (node.height // 2)))
                        if centre_dist < min_centre_dist:
                            best_axis = 'y'
                            best_index = y
                            min_centre_dist = centre_dist
                        
                        node.is_uniform = False
                    if (z + 1 < node.z + node.depth) and (current_tag != self.slices.get_block_tag(x, y, z + 1)):
                        centre_dist = abs(z - (node.z + (node.depth // 2)))
                        if centre_dist < min_centre_dist:
                            best_axis = 'z'
                            best_index = z
                            min_centre_dist = centre_dist
                            
                        node.is_uniform = False
                        
        return (best_axis, best_index)
    
    def build_kd_tree(self, node):
        """
        Build the k-d tree of compressed blocks.
        """
        
        split_axis, split_index = self.find_split(node)
        
        if node.is_uniform:
            return
        
        match split_axis:
            case 'x':
                # left <= X
                left_x, left_y, left_z = (node.x, node.y, node.z)
                left_w, left_h, left_d = (split_index - node.x + 1, node.height, node.depth)

                # right > X
                right_x, right_y, right_z = (split_index + 1, node.y, node.z)
                right_w, right_h, right_d = (node.x + node.width - (split_index + 1), node.height, node.depth)

            case 'y':
                # left <= Y
                left_x, left_y, left_z = (node.x, node.y, node.z)
                left_w, left_h, left_d = (node.width, split_index - node.y + 1, node.depth)
                
                # right > Y
                right_x, right_y, right_z = (node.x, split_index + 1, node.z)
                right_w, right_h, right_d = (node.width, node.y + node.height - (split_index + 1), node.depth)
                
            case 'z':
                # left <= Z
                left_x, left_y, left_z = (node.x, node.y, node.z)
                left_w, left_h, left_d = (node.width, node.height, split_index - node.z + 1)
                
                # right > Z
                right_x, right_y, right_z = (node.x, node.y, split_index + 1)
                right_w, right_h, right_d = (node.width, node.height, node.z + node.depth - (split_index + 1))
            
        left_tag = self.slices.get_block_tag(left_x, left_y, left_z)
        node.left = Node(left_x, left_y, left_z, left_w, left_h, left_d, left_tag, node)
        self.build_kd_tree(node.left)
        
        right_tag = self.slices.get_block_tag(right_x, right_y, right_z)
        node.right = Node(right_x, right_y, right_z, right_w, right_h, right_d, right_tag, node)
        self.build_kd_tree(node.right)
