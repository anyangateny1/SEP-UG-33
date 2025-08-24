import numpy as np

class BlockModel:
    def read_specification(self):
        """
        Reads the first line of input:
        x_count, y_count, z_count, parent_x, parent_y, parent_z
        """
        line = input().strip()
        self.x_count, self.y_count, self.z_count, self.parent_x, self.parent_y, self.parent_z = map(int, line.split(','))

    def read_tag_table(self):
        """
        Reads tag,label pairs into a dictionary until a blank line.
        Example:
            o, sea
            s, SA
        Becomes:
            {"o": "sea", "s": "SA"}
        """
        self.tag_table = {}
        while True:
            line = input().strip()
            if not line:
                break
            tag, label = line.split(', ')
            self.tag_table[tag] = label

    def read_model(self):
        """
        Reads the 3D block model from stdin.
        Stored as self.model[y, x, z].
        y=0 is the bottom row of each slice.
        """
        self.model = np.empty((self.y_count, self.x_count, self.z_count), dtype='U1')
        for z in range(self.z_count):
            slice_lines = []
            for _ in range(self.y_count):
                line = input().strip()
                slice_lines.append(line)
            # Map input lines to y rows (bottom row = y=0)
            for y, line in enumerate(slice_lines):
                for x, block in enumerate(line):
                    self.model[y, x, z] = block
            _ = input()  # consume blank line between slices

    def output_uncompressed(self):
        """
        Outputs every block in required format:
        x,y,z,1,1,1,label
        """
        for z in range(self.z_count):
            for y in range(self.y_count):
                for x in range(self.x_count):
                    tag = self.model[y, x, z]
                    label = self.tag_table[tag]
                    print(f"{x},{y},{z},1,1,1,{label}")
