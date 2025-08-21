from block_model import BlockModel

block_model = BlockModel()

block_model.read_specification()
block_model.read_tag_table()
block_model.read_model()

print(block_model.x_count, block_model.y_count, block_model.z_count, block_model.parent_x, block_model.parent_y, block_model.parent_z)

print(block_model.tag_table)

for depth in range(block_model.z_count):
    for row in range(block_model.y_count):
        for col in range(block_model.x_count):
            print(block_model.model[row, col, depth], end=' ')
        print()
    print()
