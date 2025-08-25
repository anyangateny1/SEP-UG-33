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
        
        The model is stored as a 3D NumPy array attribute:
        self.model[y, x, z]
        """
        self.model = np.empty((self.y_count, self.x_count, self.parent_z), dtype = 'U1')
        slices_count = 0
        for z in range(self.z_count): 
            for y in range(self.y_count):
                line = input()
                for x, block in enumerate(line):
                    self.model[y, x, slices_count] = block
            
            slices_count += 1
            
            # Compress slices of parent block thickness
            if slices_count == self.parent_z:
                top_slice = z+1 - slices_count
                self.compress_slices(slices_count, top_slice)
                slices_count = 0
            
            # Skip empty line
            if z < self.z_count-1:
                input()
        
        if slices_count > 0:
            top_slice = self.z_count - slices_count
            self.compress_slices(slices_count, top_slice)
        

    def compress_slices(self, num_slices, top_slice):
        """
        Outputs every block in required format:
        x,y,z,1,1,1,label
        """        
        for slice_idx in range(num_slices):
            for y in range(self.y_count):
                for x in range(self.x_count):
                    tag = self.model[y, x, slice_idx]
                    label = self.tag_table[tag]
                
                    print(f"{x},{y},{top_slice + slice_idx},1,1,1,{label}")
        
def main():
    block_model = BlockModel()

    block_model.read_specification()
    block_model.read_tag_table()
    block_model.read_model()


if __name__ == "__main__":
    main()
