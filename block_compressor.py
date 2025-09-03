import sys
import numpy as np

class RecursiveCompressor:
    def __init__(self, tag_table):
        self.tag_table = tag_table

    def compress(self, x, y, z, w, h, d, region):
        """
        Recursively compress a block region.
        Finds largest uniform block before subdividing.
        Prints output lines directly.
        """
        # Empty / padding check
        first = region[0, 0, 0]
        if first not in self.tag_table:
            return

        # Case 1: uniform block
        unique_vals = np.unique(region)
        if len(unique_vals) == 1:
            label = self.tag_table[unique_vals[0]]
            print(f"{x},{y},{z},{w},{h},{d},{label}")
            return

        # Case 2: try to find largest uniform spans along each axis
        # Check if any slices along Z are uniform
        if d > 1:
            for split in range(1, d):  # all possible z-splits
                if np.all(region[:split] == region[0, 0, 0]) and np.unique(region[:split]).size == 1:
                    # if top part uniform, emit it
                    label = self.tag_table[region[0, 0, 0]]
                    print(f"{x},{y},{z},{w},{h},{split},{label}")
                    # recursively call compress() on the remainder half
                    self.compress(x, y, z + split, w, h, d - split, region[split:])
                    return

        if h > 1:
            for split in range(1, h):  # all possible y-splits
                if np.all(region[:, :split] == region[0, 0, 0]) and np.unique(region[:, :split]).size == 1:
                    label = self.tag_table[region[0, 0, 0]]
                    print(f"{x},{y},{z},{w},{split},{d},{label}")
                    self.compress(x, y + split, z, w, h - split, d, region[:, split:])
                    return

        if w > 1:
            for split in range(1, w):  # all possible x-splits
                if np.all(region[:, :, :split] == region[0, 0, 0]) and np.unique(region[:, :, :split]).size == 1:
                    label = self.tag_table[region[0, 0, 0]]
                    print(f"{x},{y},{z},{split},{h},{d},{label}")
                    self.compress(x + split, y, z, w - split, h, d, region[:, :, split:])
                    return

        # Case 3: fallback - standard subdivision
        if w > 1 or h > 1 or d > 1:
            w2 = max(1, w // 2)
            h2 = max(1, h // 2)
            d2 = max(1, d // 2)

            for dz, z_off in enumerate([0, d2] if d > 1 else [0]):
                for dy, y_off in enumerate([0, h2] if h > 1 else [0]):
                    for dx, x_off in enumerate([0, w2] if w > 1 else [0]):
                        sub_w = w2 if dx == 0 else w - w2
                        sub_h = h2 if dy == 0 else h - h2
                        sub_d = d2 if dz == 0 else d - d2

                        sub_region = region[
                            z_off:z_off+sub_d,
                            y_off:y_off+sub_h,
                            x_off:x_off+sub_w
                        ]

                        self.compress(
                            x + x_off, y + y_off, z + z_off,
                            sub_w, sub_h, sub_d,
                            sub_region
                        )
        else:
            # Case 4: single voxel
            label = self.tag_table[region[0, 0, 0]]



class BlockModel:
    def read_specification(self):
        line = sys.stdin.readline().strip()
        self.x_count, self.y_count, self.z_count, self.parent_x, self.parent_y, self.parent_z = map(int, line.split(','))

    def read_tag_table(self):
        self.tag_table = {}
        while True:
            line = sys.stdin.readline().strip()
            if not line:
                break
            tag, label = line.split(', ')
            self.tag_table[tag] = label

    def read_model(self):
        self.model = np.full((self.parent_z, self.y_count, self.x_count), '?', dtype='U1')

        top_slice = 0
        for z in range(self.z_count):
            for y in range(self.y_count):
                line = sys.stdin.readline().rstrip()
                for x, block_tag in enumerate(line):
                    self.model[z % self.parent_z, y, x] = block_tag
            if z < self.z_count - 1:
                sys.stdin.readline()  # skip blank line

            if (z + 1) % self.parent_z == 0:
                self.compress_slices(top_slice, self.parent_z)
                top_slice = z + 1

        if self.z_count % self.parent_z != 0:
            self.compress_slices(top_slice, self.z_count % self.parent_z)

    def compress_slices(self, top_slice, n_slices):
        compressor = RecursiveCompressor(self.tag_table)

        for y in range(0, self.y_count, self.parent_y):
            for x in range(0, self.x_count, self.parent_x):
                z = top_slice
                width = min(self.parent_x, self.x_count - x)
                height = min(self.parent_y, self.y_count - y)
                depth = n_slices

                region = self.model[:depth, y:y+height, x:x+width]
                compressor.compress(x, y, z, width, height, depth, region)


def main():
    block_model = BlockModel()
    block_model.read_specification()
    block_model.read_tag_table()
    block_model.read_model()


if __name__ == "__main__":
    main()
