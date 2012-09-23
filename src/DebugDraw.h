#pragma once

#include "Utils.h"

class DebugDraw
{
private:
    int currentMipMapLevel;
    int maxMipMapLevel;
    int voxelGridLength;
    GLuint voxelTexture;
    GLuint vertexArray;
    GLuint voxelBuffer;
    GLuint voxelDebugProgram;
    static const unsigned int numVerticesCube = 8;
    static const unsigned int numElementsCube = 36; 

    struct MipMapInfo
    {
        unsigned int offset;
        unsigned int numVoxels;
    };

    std::vector<MipMapInfo> mipMapInfoArray;

    struct Voxel
    {
        // .xyz are translation, .w is scale
        glm::vec4 transformation;
        glm::vec4 color;
    };

public:
    DebugDraw(){}
    virtual ~DebugDraw(){}

    void begin(GLuint voxelTexture, int voxelGridLength)
    {
        this->voxelGridLength = voxelGridLength;
        this->voxelTexture = voxelTexture;
        this->maxMipMapLevel = Utils::getNumMipMapLevels(voxelGridLength);

        // Create buffer objects and vao
        glm::vec3 vertices[numVerticesCube] = {glm::vec3(-.5, -.5, -.5), glm::vec3(-.5, -.5, .5), glm::vec3(-.5, .5, .5), glm::vec3(-.5, .5, -.5), glm::vec3(.5, .5, -.5), glm::vec3(.5, -.5, -.5), glm::vec3(.5, .5, .5), glm::vec3(.5, -.5, .5)};

        unsigned short elements[numElementsCube] = {0, 1, 2, 0, 2, 3, 3, 4, 5, 3, 5, 0, 4, 6, 7, 4, 7, 5, 1, 7, 6, 1, 6, 2, 1, 0, 5, 1, 5, 7, 6, 4, 3, 6, 3, 2};

        GLuint vertexBuffer;
        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*numVerticesCube, vertices, GL_STATIC_DRAW);

        GLuint elementArrayBuffer;
        glGenBuffers(1, &elementArrayBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementArrayBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short)*numElementsCube, elements, GL_STATIC_DRAW);

        glGenBuffers(1, &voxelBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, voxelBuffer);
        // Don't fill this buffer yet

        glGenVertexArrays(1, &vertexArray);
        glBindVertexArray(vertexArray);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glEnableVertexAttribArray(POSITION_ATTR);
        glVertexAttribPointer(POSITION_ATTR, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glBindBuffer(GL_ARRAY_BUFFER, voxelBuffer);
        glEnableVertexAttribArray(DEBUG_TRANSFORM_ATTR);
        glEnableVertexAttribArray(DEBUG_COLOR_ATTR);
        glVertexAttribPointer(DEBUG_TRANSFORM_ATTR, 4, GL_FLOAT, GL_FALSE, sizeof(Voxel), (void*)(0));
        glVertexAttribPointer(DEBUG_COLOR_ATTR, 4, GL_FLOAT, GL_FALSE, sizeof(Voxel), (void*)(sizeof(glm::vec4)));
        glVertexAttribDivisor(DEBUG_TRANSFORM_ATTR, 1);
        glVertexAttribDivisor(DEBUG_COLOR_ATTR, 1);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementArrayBuffer);

        //Unbind everything
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // Create shader program
        GLuint vertexShaderObject = glf::createShader(GL_VERTEX_SHADER, "src/voxelDebug.vert");
        GLuint fragmentShaderObject = glf::createShader(GL_FRAGMENT_SHADER, "src/voxelDebug.frag");

        voxelDebugProgram = glCreateProgram();
        glAttachShader(voxelDebugProgram, vertexShaderObject);
        glAttachShader(voxelDebugProgram, fragmentShaderObject);
        glDeleteShader(vertexShaderObject);
        glDeleteShader(fragmentShaderObject);

        glLinkProgram(voxelDebugProgram);
        glf::checkProgram(voxelDebugProgram);

        createCubesFromVoxels();
    }
    void createCubesFromVoxels()
    {
        std::vector<Voxel> voxelArray;

        glBindTexture(GL_TEXTURE_3D, voxelTexture);
        
        int mipMapVoxelGridLength = this->voxelGridLength;
        float voxelScale = 1.0f / mipMapVoxelGridLength;
        for(int i = 0; i < this->maxMipMapLevel; i++)
        {
            MipMapInfo mipMapInfo;
            mipMapInfo.offset = voxelArray.size();

            std::vector<glm::u8vec4> imageData(mipMapVoxelGridLength*mipMapVoxelGridLength*mipMapVoxelGridLength);
            glGetTexImage(GL_TEXTURE_3D, i, GL_RGBA, GL_UNSIGNED_BYTE, &imageData[0]);

            // apply an offset to the position because the origin of the cube model is in its center rather than a corner
            glm::vec3 offset = glm::vec3(-mipMapVoxelGridLength * voxelScale / 2.0f) + glm::vec3(voxelScale/2);

            unsigned int textureIndex = 0;
            for(int j = 0; j < mipMapVoxelGridLength; j++)
            for(int k = 0; k < mipMapVoxelGridLength; k++)
            for(int l = 0; l < mipMapVoxelGridLength; l++)
            {
                glm::vec3 position = glm::vec3(j*voxelScale,k*voxelScale,l*voxelScale) + offset;
                glm::u8vec4 color = imageData[textureIndex];
                if(color.a != 0)
                {
                    Voxel voxel;
                    voxel.color = glm::vec4(color)/255.0f;
                    voxel.transformation = glm::vec4(position, voxelScale);
                    voxelArray.push_back(voxel);
                }
                textureIndex++;
            }
            mipMapInfo.numVoxels = voxelArray.size() - mipMapInfo.offset;
            mipMapInfoArray.push_back(mipMapInfo);
            voxelScale *= 2;
            mipMapVoxelGridLength /= 2;
        }

        glBindBuffer(GL_ARRAY_BUFFER, voxelBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Voxel)*voxelArray.size(), &voxelArray[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    virtual void display()
    {
        unsigned int baseInstance = mipMapInfoArray[this->currentMipMapLevel].offset;
        unsigned int primCount = mipMapInfoArray[this->currentMipMapLevel].numVoxels;

        glUseProgram(voxelDebugProgram);
        glBindVertexArray(vertexArray);
        glDrawElementsInstancedBaseInstance(GL_TRIANGLES, numElementsCube, GL_UNSIGNED_SHORT, 0, primCount, baseInstance);
    }

    virtual void keyboardEvent(unsigned char keyCode)
    {

        if(keyCode == 45)
        {
            this->setMipMapLevel(this->getMipMapLevel() - 1);
        }
        else if(keyCode == 43)
        {
            this->setMipMapLevel(this->getMipMapLevel() + 1);
        }
    }
    
    unsigned int getMipMapLevel()
    {
        return this->currentMipMapLevel;
    }

    bool setMipMapLevel(int level)
    {
        this->currentMipMapLevel = std::min(std::max(0, level), this->maxMipMapLevel - 1);
        return (this->currentMipMapLevel != level);
    }
};