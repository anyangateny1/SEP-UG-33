from block import Block
from block_growth import BlockGrowth

import numpy as np

class BlockModel:
    def read_specification(self):
        """
        Reads in the specification of the map from stdin.
        
        The input is a line of comma seperated ints in the format:
            x_count, y_count, z_count, parent_x, parent_y, parent_z
        
        The values are stored as int attributes of the object.
        """
        line = input()
        line = line.split(',')
        
        self.x_count, self.y_count, self.z_count, self.parent_x, self.parent_y, self.parent_z = map(int, line)

    def read_tag_table(self):
        """
        Reads in the tag table from stdin until an empty line is inputted.
        
        Each line of input is a tag and label pair in the format:
            tag, label
        
        The tag table is stored as a dictionary:
            self.tag_table[tag] = label
        """
        self.tag_table = {}
        line = input()
        while line:
            line = line.split(', ')
            self.tag_table[line[0]] = line[1]
            
            line = input()

    def read_model(self):
        """
        Reads the 3D block model from stdin.
        Each slice of the model is seperated by an empty line in input.
        
        Stores depth slices of up to parent block thickness. 
        
        The model is stored as a 3D NumPy array attribute:
        self.model[y, x, z]
        """        
        self.model = np.empty((self.parent_z, self.y_count, self.x_count), dtype = 'U1')
        
        top_slice = 0
        n_slices = self.parent_z
        for z in range(self.z_count): 
            for y in range(self.y_count):
                line = input()
                for x, block_tag in enumerate(line):
                    self.model[z % self.parent_z, y, x] = block_tag
            
            # Compress slices of parent block thickness
            if (z+1) % self.parent_z == 0:
                self.compress_slices(top_slice, n_slices)
                top_slice = z + 1
            
            # Skip empty line
            if z < self.z_count-1:
                input()
        
        # Remaining slices less than parent block thickenss
        if self.z_count % self.parent_z != 0:
            n_slices = self.z_count % self.parent_z
            self.compress_slices(top_slice, n_slices)
        

    def compress_slices(self, top_slice, n_slices):
        """
        Compresses a section of slices of parent block thickness.
        """
        for y in range(0, self.y_count, self.parent_y):
            for x in range(0, self.x_count, self.parent_x):
                z = top_slice
                width = min(self.parent_x, self.x_count - x)
                height = min(self.parent_y, self.y_count - y)
                depth = n_slices
                tag = self.model[z % self.parent_z, y, x]
                
                parent_block = Block(x, y, z, width, height, depth, tag)
                model_slices = self.model[:depth, y:parent_block.y_end, x:parent_block.x_end]
                block_growth = BlockGrowth(model_slices, self.tag_table)
                
                block_growth.run(parent_block)
                
        
def main():
    block_model = BlockModel()

    block_model.read_specification()
    block_model.read_tag_table()
    block_model.read_model()


if __name__ == "__main__":
    main()
