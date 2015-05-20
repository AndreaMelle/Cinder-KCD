#include "KCDDevice.h"

using namespace kcd;

KCDRenderer::KCDRenderer()
{
}

KCDRenderer::~KCDRenderer()
{
}

void KCDRenderer::update(std::weak_ptr<KCDDevice> device)
{
	auto devicePtr = device.lock();

	if (devicePtr && glIsTexture(colorTextureName))
	{
		glBindTexture(GL_TEXTURE_2D, colorTextureName);
		devicePtr->mImageDataMutex.lock();
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ColorFrameWidth, ColorFrameHeight, GL_BGRA, GL_UNSIGNED_BYTE, devicePtr->mColorBuffer);
		devicePtr->mImageDataMutex.unlock();
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

void KCDRenderer::setup(std::weak_ptr<KCDDevice> device)
{
	glGenTextures(1, &colorTextureName);
	glBindTexture(GL_TEXTURE_2D, colorTextureName);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, ColorFrameWidth);

	auto devicePtr = device.lock();
	if (devicePtr)
	{
		devicePtr->mImageDataMutex.lock();
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ColorFrameWidth, ColorFrameHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, devicePtr->mColorBuffer);
		devicePtr->mImageDataMutex.unlock();
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ColorFrameWidth, ColorFrameHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);
	}

	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void KCDRenderer::teardown(std::weak_ptr<KCDDevice> device)
{
	glDeleteTextures(1, &colorTextureName);
}

GLuint KCDRenderer::getColorTextureRef()
{
	return colorTextureName;
}

