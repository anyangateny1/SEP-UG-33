from block import Block

class Node(Block):
    def __init__(self, x, y, z, width, height, depth, tag, parent=None):
        super().__init__(x, y, z, width, height, depth, tag)
        self.parent = parent
        self.left = None
        self.right = None