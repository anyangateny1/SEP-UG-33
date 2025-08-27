from slices_section import SlicesSection
from block import Block
from node import Node
from kd_tree import KdTree

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
        self.slices = SlicesSection(self.x_count, self.y_count, self.parent_z)
        slices_count = 0
        for z in range(self.z_count): 
            for y in range(self.y_count):
                line = input()
                for x, block_tag in enumerate(line):
                    self.slices.set_block_tag(x, y, z, block_tag)
            
            # Compress slices of parent block thickness
            if (z+1) % self.parent_z == 0:
                self.compress_slices()
                self.slices.top_slice = z + 1
            
            # Skip empty line
            if z < self.z_count-1:
                input()
        
        # Remaining slices less than parent block thickenss
        if self.z_count % self.parent_z != 0:
            self.slices.n = self.z_count % self.parent_z
            self.compress_slices()
        

    def compress_slices(self):
        """
        Outputs every block in required format:
        x,y,z,1,1,1,label
        """
        for y in range(0, self.y_count, self.parent_y):
            for x in range(0, self.x_count, self.parent_x):
                z = self.slices.top_slice
                width = min(self.parent_x, self.x_count - x)
                height = min(self.parent_y, self.y_count - y)
                depth = self.slices.n
                tag = self.slices.get_block_tag(x, y, z)
                
                node = Node(x, y, z, width, height, depth, tag)
                kd_tree = KdTree(self.slices, self.tag_table, self.parent_x, self.parent_y, self.parent_z)
                kd_tree.build_kd_tree(node)
                
                kd_tree.post_compression_pass(node)
        
def main():
    block_model = BlockModel()

    block_model.read_specification()
    block_model.read_tag_table()
    block_model.read_model()


if __name__ == "__main__":
    main()
