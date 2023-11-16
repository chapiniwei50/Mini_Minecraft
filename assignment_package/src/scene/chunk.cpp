#include "chunk.h"

Chunk::Chunk(int x, int z, OpenGLContext* context)
    : Drawable(context), m_blocks(), minX(x), minZ(z), m_neighbors{{XPOS, nullptr}, {XNEG, nullptr}, {ZPOS, nullptr}, {ZNEG, nullptr}}
{
    std::fill_n(m_blocks.begin(), 65536, EMPTY);
}

// Does bounds checking with at()
BlockType Chunk::getBlockAt(unsigned int x, unsigned int y, unsigned int z) const {
    return m_blocks.at(x + 16 * y + 16 * 256 * z);
}

// Exists to get rid of compiler warnings about int -> unsigned int implicit conversion
BlockType Chunk::getBlockAt(int x, int y, int z) const {
    return getBlockAt(static_cast<unsigned int>(x), static_cast<unsigned int>(y), static_cast<unsigned int>(z));
}

// Does bounds checking with at()
void Chunk::setBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t) {
    m_blocks.at(x + 16 * y + 16 * 256 * z) = t;
}


const static std::unordered_map<Direction, Direction, EnumHash> oppositeDirection {
    {XPOS, XNEG},
    {XNEG, XPOS},
    {YPOS, YNEG},
    {YNEG, YPOS},
    {ZPOS, ZNEG},
    {ZNEG, ZPOS}
};

void Chunk::linkNeighbor(uPtr<Chunk> &neighbor, Direction dir) {
    if(neighbor != nullptr) {
        this->m_neighbors[dir] = neighbor.get();
        neighbor->m_neighbors[oppositeDirection.at(dir)] = this;
    }
}

int Chunk::is_boundary(int x, int y, int z) const
{
    // check whether the block at (x, z, y) is a boundary block that need to be rendered
    // return a six-bit binary number, each bit indicates whether a certain face is boundary
    // [xneg, xpos, yneg, ypos, zneg, zpos], 1 means is_boundary

    int res = 0b000000;

    // empty cannot be boundaries
    if (getBlockAt(x, y, z) == EMPTY)
        return res;

    // x neg face direction
    if ((x == 0 && m_neighbors.at(XNEG) == nullptr) ||
        (x == 0 && m_neighbors.at(XNEG)->getBlockAt(15, y, z) == EMPTY) ||
        (x != 0 && getBlockAt(x - 1, y, z) == EMPTY))
        res = res | 0b100000;

    // x pos face direction
    if ((x == 15 && m_neighbors.at(XPOS) == nullptr) ||
        (x == 15 && m_neighbors.at(XPOS)->getBlockAt(0, y, z) == EMPTY) ||
        (x != 15 && getBlockAt(x + 1, y, z) == EMPTY))
        res = res | 0b010000;

    // y neg face direction
    if (y == 0 ||
       (y != 0 && getBlockAt(x, y - 1, z) == EMPTY))
        res = res | 0b001000;

    // y pos face direction
    if (y == 255 ||
       (y != 255 && getBlockAt(x, y + 1, z) == EMPTY))
        res = res | 0b000100;

    // z neg face direction
    if ((z == 0 && m_neighbors.at(ZNEG) == nullptr) ||
        (z == 0 && m_neighbors.at(ZNEG)->getBlockAt(x, y, 15) == EMPTY) ||
        (z != 0 && getBlockAt(x, y, z - 1) == EMPTY))
        res = res | 0b000010;

    // z pos face direction
    if ((z == 15 && m_neighbors.at(ZPOS) == nullptr) ||
        (z == 15 && m_neighbors.at(ZPOS)->getBlockAt(x, y, 0) == EMPTY) ||
        (z != 15 && getBlockAt(x, y, z + 1) == EMPTY))
        res = res | 0b000001;

    return res;
}


void Chunk::createVBOdata()
{
    std::vector<glm::vec4> pos_nor_color;
    std::vector<GLuint> idx;

    for (unsigned int x = 0; x < 16; x++)
        for (unsigned int y = 0; y < 256; y++)
            for (unsigned int z = 0; z < 16; z++)
            {
                int boundary_info = is_boundary(x, y, z);

                // empty block or no boundaries. No need to draw
                if (boundary_info == 0)
                    continue;

                BlockType t = getBlockAt(x, y, z);
                glm::vec4 color;
                switch (t) {
                case GRASS:
                    color = glm::vec4(95.f, 159.f, 53.f, 255.f) / 255.f;
                    break;
                case DIRT:
                    color = glm::vec4(121.f, 85.f, 58.f, 255.f) / 255.f;
                    break;
                case STONE:
                    color = glm::vec4(0.5f);
                    break;
                case WATER:
                    color = glm::vec4(0.f, 0.f, 0.75f, 1.f);
                    break;
                default:
                    color = glm::vec4(1.f, 0.f, 1.f, 1.f);
                    break;
                }

                // temporarily differentiate the color a bit for better visualization
                float ratio = ((float)std::rand()) / RAND_MAX;
                ratio = ratio * 0.4 + 0.8;
                color *= ratio;
                color[3] = 1.f;

                // draw x neg face
                if ((boundary_info & 0b100000) != 0)
                {
                    pos_nor_color.push_back(glm::vec4(0.0 + x + minX, 1.0 + y, 1.0 + z + minZ, 1.0));
                    pos_nor_color.push_back(glm::vec4(-1.0, 0.0, 0.0, 0.0));
                    pos_nor_color.push_back(color);

                    pos_nor_color.push_back(glm::vec4(0.0 + x + minX, 0.0 + y, 1.0 + z + minZ, 1.0));
                    pos_nor_color.push_back(glm::vec4(-1.0, 0.0, 0.0, 0.0));
                    pos_nor_color.push_back(color);

                    pos_nor_color.push_back(glm::vec4(0.0 + x + minX, 0.0 + y, 0.0 + z + minZ, 1.0));
                    pos_nor_color.push_back(glm::vec4(-1.0, 0.0, 0.0, 0.0));
                    pos_nor_color.push_back(color);

                    pos_nor_color.push_back(glm::vec4(0.0 + x + minX, 1.0 + y, 0.0 + z + minZ, 1.0));
                    pos_nor_color.push_back(glm::vec4(-1.0, 0.0, 0.0, 0.0));
                    pos_nor_color.push_back(color);
                }

                // draw x pos face
                if ((boundary_info & 0b010000) != 0)
                {
                    pos_nor_color.push_back(glm::vec4(1.0 + x + minX, 1.0 + y, 0.0 + z + minZ, 1.0));
                    pos_nor_color.push_back(glm::vec4(1.0, 0.0, 0.0, 0.0));
                    pos_nor_color.push_back(color);

                    pos_nor_color.push_back(glm::vec4(1.0 + x + minX, 0.0 + y, 0.0 + z + minZ, 1.0));
                    pos_nor_color.push_back(glm::vec4(1.0, 0.0, 0.0, 0.0));
                    pos_nor_color.push_back(color);

                    pos_nor_color.push_back(glm::vec4(1.0 + x + minX, 0.0 + y, 1.0 + z + minZ, 1.0));
                    pos_nor_color.push_back(glm::vec4(1.0, 0.0, 0.0, 0.0));
                    pos_nor_color.push_back(color);

                    pos_nor_color.push_back(glm::vec4(1.0 + x + minX, 1.0 + y, 1.0 + z + minZ, 1.0));
                    pos_nor_color.push_back(glm::vec4(1.0, 0.0, 0.0, 0.0));
                    pos_nor_color.push_back(color);
                }

                // draw y neg face
                if ((boundary_info & 0b001000) != 0)
                {
                    pos_nor_color.push_back(glm::vec4(1.0 + x + minX, 0.0 + y, 1.0 + z + minZ, 1.0));
                    pos_nor_color.push_back(glm::vec4(0.0, -1.0, 0.0, 0.0));
                    pos_nor_color.push_back(color);

                    pos_nor_color.push_back(glm::vec4(1.0 + x + minX, 0.0 + y, 0.0 + z + minZ, 1.0));
                    pos_nor_color.push_back(glm::vec4(0.0, -1.0, 0.0, 0.0));
                    pos_nor_color.push_back(color);

                    pos_nor_color.push_back(glm::vec4(0.0 + x + minX, 0.0 + y, 0.0 + z + minZ, 1.0));
                    pos_nor_color.push_back(glm::vec4(0.0, -1.0, 0.0, 0.0));
                    pos_nor_color.push_back(color);

                    pos_nor_color.push_back(glm::vec4(0.0 + x + minX, 0.0 + y, 1.0 + z + minZ, 1.0));
                    pos_nor_color.push_back(glm::vec4(0.0, -1.0, 0.0, 0.0));
                    pos_nor_color.push_back(color);
                }

                // draw y pos face
                if ((boundary_info & 0b000100) != 0)
                {
                    pos_nor_color.push_back(glm::vec4(1.0 + x + minX, 1.0 + y, 0.0 + z + minZ, 1.0));
                    pos_nor_color.push_back(glm::vec4(0.0, 1.0, 0.0, 0.0));
                    pos_nor_color.push_back(color);

                    pos_nor_color.push_back(glm::vec4(1.0 + x + minX, 1.0 + y, 1.0 + z + minZ, 1.0));
                    pos_nor_color.push_back(glm::vec4(0.0, 1.0, 0.0, 0.0));
                    pos_nor_color.push_back(color);

                    pos_nor_color.push_back(glm::vec4(0.0 + x + minX, 1.0 + y, 1.0 + z + minZ, 1.0));
                    pos_nor_color.push_back(glm::vec4(0.0, 1.0, 0.0, 0.0));
                    pos_nor_color.push_back(color);

                    pos_nor_color.push_back(glm::vec4(0.0 + x + minX, 1.0 + y, 0.0 + z + minZ, 1.0));
                    pos_nor_color.push_back(glm::vec4(0.0, 1.0, 0.0, 0.0));
                    pos_nor_color.push_back(color);
                }

                // draw z neg face
                if ((boundary_info & 0b000010) != 0)
                {
                    pos_nor_color.push_back(glm::vec4(0.0 + x + minX, 1.0 + y, 0.0 + z + minZ, 1.0));
                    pos_nor_color.push_back(glm::vec4(0.0, 0.0, -1.0, 0.0));
                    pos_nor_color.push_back(color);

                    pos_nor_color.push_back(glm::vec4(0.0 + x + minX, 0.0 + y, 0.0 + z + minZ, 1.0));
                    pos_nor_color.push_back(glm::vec4(0.0, 0.0, -1.0, 0.0));
                    pos_nor_color.push_back(color);

                    pos_nor_color.push_back(glm::vec4(1.0 + x + minX, 0.0 + y, 0.0 + z + minZ, 1.0));
                    pos_nor_color.push_back(glm::vec4(0.0, 0.0, -1.0, 0.0));
                    pos_nor_color.push_back(color);

                    pos_nor_color.push_back(glm::vec4(1.0 + x + minX, 1.0 + y, 0.0 + z + minZ, 1.0));
                    pos_nor_color.push_back(glm::vec4(0.0, 0.0, -1.0, 0.0));
                    pos_nor_color.push_back(color);
                }

                // draw z pos face
                if ((boundary_info & 0b000001) != 0)
                {
                    pos_nor_color.push_back(glm::vec4(1.0 + x + minX, 1.0 + y, 1.0 + z + minZ, 1.0));
                    pos_nor_color.push_back(glm::vec4(0.0, 0.0, 1.0, 0.0));
                    pos_nor_color.push_back(color);

                    pos_nor_color.push_back(glm::vec4(1.0 + x + minX, 0.0 + y, 1.0 + z + minZ, 1.0));
                    pos_nor_color.push_back(glm::vec4(0.0, 0.0, 1.0, 0.0));
                    pos_nor_color.push_back(color);

                    pos_nor_color.push_back(glm::vec4(0.0 + x + minX, 0.0 + y, 1.0 + z + minZ, 1.0));
                    pos_nor_color.push_back(glm::vec4(0.0, 0.0, 1.0, 0.0));
                    pos_nor_color.push_back(color);

                    pos_nor_color.push_back(glm::vec4(0.0 + x + minX, 1.0 + y, 1.0 + z + minZ, 1.0));
                    pos_nor_color.push_back(glm::vec4(0.0, 0.0, 1.0, 0.0));
                    pos_nor_color.push_back(color);
                }
            }

    // generate index data according to the number of face to render
    int num_faces = pos_nor_color.size() / 4 / 3;
    for (int i = 0; i < num_faces; i++)
    {
        idx.push_back(i * 4);
        idx.push_back(i * 4 + 1);
        idx.push_back(i * 4 + 2);
        idx.push_back(i * 4);
        idx.push_back(i * 4 + 2);
        idx.push_back(i * 4 + 3);
    }

    m_count = idx.size();

    // buff vertex data and indices into proper VBOs.
    buff_data(pos_nor_color, idx);
}


void Chunk::buff_data(std::vector<glm::vec4> &pos_nor_color, std::vector<GLuint> &idx)
{
    // buff vertex data and indices into proper VBOs.
    generateIdx();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdx);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(GLuint), idx.data(), GL_STATIC_DRAW);

    generateInterleaved();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufInterleaved);
    mp_context->glBufferData(GL_ARRAY_BUFFER, pos_nor_color.size() * sizeof(glm::vec4), pos_nor_color.data(), GL_STATIC_DRAW);


}

void createBlockdata(){

}
