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
        Rows are ordered bottom to top in the input (y=0 at the bottom).
        Slices are ordered bottom to top in the input (z=0 is first).
        Columns are ordered left to right in the input.
        
        The model is stored as a 3D NumPy array attribute (input order preserved):
        self.model[y, x, z]
        """
        self.model = np.empty((self.y_count, self.x_count, self.z_count), dtype = 'U1')
        for depth in range(self.z_count):
            for row in range(self.y_count):
                line = input()
                for col, block in enumerate(line):
                    self.model[row, col, depth] = block
            input()

    def output_model(self):
        """
        Outputs every block in required format:
        x,y,z,1,1,1,label
        """
        for depth in range(self.z_count):
            for row in range(self.y_count):
                for col in range(self.x_count):
                    tag = self.model[row, col, depth]
                    label = self.tag_table[tag]
                    print(f"{col},{(self.y_count-1) - row},{(self.z_count-1) - depth},1,1,1,{label}")
                    
def main():
    block_model = BlockModel()

    block_model.read_specification()
    block_model.read_tag_table()
    block_model.read_model()
    block_model.output_model()


if __name__ == "__main__":
    main()
