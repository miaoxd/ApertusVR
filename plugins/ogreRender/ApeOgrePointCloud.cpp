#include "ApeOgrePointCloud.h"

Ape::OgrePointCloud::OgrePointCloud(const std::string& name, const std::string& resourcegroup, const int numpoints, float *parray, float *carray, float boundigSphereRadius)
{
	mRenderSystemForVertex = Ogre::Root::getSingleton().getRenderSystem();
	Ogre::MeshPtr msh = Ogre::MeshManager::getSingleton().createManual(name, resourcegroup);
	Ogre::SubMesh* sub = msh->createSubMesh();
	msh->sharedVertexData = new Ogre::VertexData();
	msh->sharedVertexData->vertexCount = numpoints;
	Ogre::VertexDeclaration* decl = msh->sharedVertexData->vertexDeclaration;
	decl->addElement(0, 0, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
	mVbuf = Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(decl->getVertexSize(0), msh->sharedVertexData->vertexCount, Ogre::HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
	mVbuf->writeData(0, mVbuf->getSizeInBytes(), parray, true);
	if (carray != NULL)
	{
		decl->addElement(1, 0, Ogre::VET_COLOUR, Ogre::VES_DIFFUSE);
		mCbuf = Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(Ogre::VertexElement::getTypeSize(Ogre::VET_COLOUR),msh->sharedVertexData->vertexCount, Ogre::HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
		Ogre::RenderSystem* rs = Ogre::Root::getSingleton().getRenderSystem();
		Ogre::RGBA *colours = new Ogre::RGBA[numpoints];
		for (int i = 0, k = 0; i < numpoints * 3, k < numpoints; i += 3, k++)
		{
			Ogre::ColourValue color(carray[i], carray[i + 1], carray[i + 2]);
			rs->convertColourValue(color, &colours[k]);
		}
		mCbuf->writeData(0, mCbuf->getSizeInBytes(), colours, true);
		delete[] colours;
	}
	Ogre::VertexBufferBinding* bind = msh->sharedVertexData->vertexBufferBinding;
	bind->setBinding(0, mVbuf);
	if (carray != NULL)
	{
		bind->setBinding(1, mCbuf);
	}
	sub->useSharedVertices = true;
	sub->operationType = Ogre::RenderOperation::OT_POINT_LIST;
	msh->_setBounds(Ogre::AxisAlignedBox(Ogre::Vector3(-boundigSphereRadius, -boundigSphereRadius, -boundigSphereRadius), Ogre::Vector3(boundigSphereRadius, boundigSphereRadius, boundigSphereRadius)), true);
	//msh->_setBoundingSphereRadius(boundigSphereRadius);
	msh->load();
}

Ape::OgrePointCloud::~OgrePointCloud()
{

}

void Ape::OgrePointCloud::updateVertexPositions(int size, float *points)
{
	float *pPArray = static_cast<float*>(mVbuf->lock(Ogre::HardwareBuffer::HBL_DISCARD));
	for (int i = 0; i < size * 3; i += 3)
	{
		pPArray[i] = points[i];
		pPArray[i + 1] = points[i + 1];
		pPArray[i + 2] = points[i + 2];
	}
	mVbuf->unlock();
}

void Ape::OgrePointCloud::updateVertexColours(int size, float *colours)
{
	Ogre::RGBA *colorArrayBuffer = static_cast<Ogre::RGBA*>(mCbuf->lock(Ogre::HardwareBuffer::HBL_DISCARD));
	int j = 0;
	for (int i = 0; i < size * 3; i += 3)
	{
		Ogre::ColourValue color = Ogre::ColourValue(colours[i], colours[i + 1], colours[i + 2]);
		mRenderSystemForVertex->convertColourValue(color, &colorArrayBuffer[j++]);
	}
	mCbuf->unlock();
}