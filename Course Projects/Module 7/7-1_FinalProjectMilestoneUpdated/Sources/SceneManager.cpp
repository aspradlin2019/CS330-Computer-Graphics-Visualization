///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	// clear the allocated memory
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
	// destroy the created OpenGL textures
	DestroyGLTextures();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureSlot = FindTextureSlot(textureTag);
		if (textureSlot >= 0)
		{
			glActiveTexture(GL_TEXTURE0 + textureSlot);
			glBindTexture(GL_TEXTURE_2D, m_textureIDs[textureSlot].ID);
			m_pShaderManager->setSampler2DValue(g_TextureValueName, textureSlot);
		}
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***************************************************************
*  LoadSceneTextures()                                         *
*                                                              *
*  This method is used for preparing the 3D scene by loading   *
*  the shapes, textures in memory to support the 3D scene      *
*  rendering                                                   *
****************************************************************/
void SceneManager::LoadSceneTextures()
{
	// Load wood texture for the bead maze base, ring            
	// stacker base, and ring stacker vertical rod               
	CreateGLTexture("../../../../CS330 Content/CS330Content/Projects/7-1_FinalProjectMilestones/Source/textures/oakwood.jpg", "oakWood");

	// Load metal texture for the bead maze rods
	CreateGLTexture("../../../../CS330 Content/CS330Content/Projects/7-1_FinalProjectMilestones/Source/textures/metal.jpg", "metalTexture");


	// Load steel texture for the bead maze rods
	CreateGLTexture("../../../../CS330 Content/CS330Content/Projects/7-1_FinalProjectMilestones/Source/textures/stainless.jpg", "steelTexture");

	/*************************************************************
	*    Load different colored plastic textures for the bead    *
	*    maze beads and the rings of the ring stacker            *
	**************************************************************/

	// Load light blue plastic texture
	CreateGLTexture("../../../../CS330 Content/CS330Content/Projects/7-1_FinalProjectMilestones/Source/textures/lightblueplastic.jpg", "ltbluePlastic");

	// Load blue plastic texture
	CreateGLTexture("../../../../CS330 Content/CS330Content/Projects/7-1_FinalProjectMilestones/Source/textures/blueplastic.jpg", "bluePlastic");

	// Load magenta plastic texture
	CreateGLTexture("../../../../CS330 Content/CS330Content/Projects/7-1_FinalProjectMilestones/Source/textures/magentaplastic.jpg", "magentaPlastic");

	// Load red plastic texture
	CreateGLTexture("../../../../CS330 Content/CS330Content/Projects/7-1_FinalProjectMilestones/Source/textures/redplastic.jpg", "redPlastic");

	// Load yellow-orange plastic texture
	CreateGLTexture("../../../../CS330 Content/CS330Content/Projects/7-1_FinalProjectMilestones/Source/textures/orangeplastic.jpg", "orangePlastic");

	// Load green plastic texture
	CreateGLTexture("../../../../CS330 Content/CS330Content/Projects/7-1_FinalProjectMilestones/Source/textures/greenplastic.jpg", "greenPlastic");

	// Load ash wood texture
	CreateGLTexture("../../../../CS330 Content/CS330Content/Projects/7-1_FinalProjectMilestones/Source/textures/ashwood.jpg", "ashWood");

	// Load letterA texture
	CreateGLTexture("../../../../CS330 Content/CS330Content/Projects/7-1_FinalProjectMilestones/Source/textures/letterA.png", "letterA");
	
	// Load letterB texture
	CreateGLTexture("../../../../CS330 Content/CS330Content/Projects/7-1_FinalProjectMilestones/Source/textures/letterB.png", "letterB");

	// Load letterC texture
	CreateGLTexture("../../../../CS330 Content/CS330Content/Projects/7-1_FinalProjectMilestones/Source/textures/letterC.png", "letterC");

	// Bind all loaded textures
	BindGLTextures();
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the textures for the 3D scene
	LoadSceneTextures();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	// Load the plane mesh (used for the floor and background)
	m_basicMeshes->LoadPlaneMesh();

	// Load the cylinder mesh (used for the vertical rod)
	m_basicMeshes->LoadCylinderMesh();

	// Load the box mesh (used for the bead maze base)
	m_basicMeshes->LoadBoxMesh();

	/******************************************************************/
	/*** Load the torus mesh with set and unset thickness           ***/
	/*** Used for stackable rings in ring stacker recreation        ***/
	/*** and curves in rod of bead maze                             ***/
	/******************************************************************/

	// Set tube thickness to 0.3f for torus mesh
	m_basicMeshes->LoadTorusMesh(0.3f);

	m_basicMeshes->LoadExtraTorusMesh1(0.35f);

	// Load the newly created quarter torus mesh 
	// (used for the rod curves the beads are on in the bead maze)
	m_basicMeshes->DrawQuarterTorusMesh(0.2f);

	// Load the sphere mesh (used for the bead-maze beads 
	m_basicMeshes->LoadSphereMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	/******************************************************************/
	/*** Floor Mesh                                                 ***/
	/******************************************************************/

	// set the XYZ scale for the floor mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the floor mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the floor mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	// White
	SetShaderColor(1, 1, 1, 1);

	// draw the floor mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/


	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	/******************************************************************/
	/*** Background Mesh                                            ***/
	/******************************************************************/
	// set the XYZ scale for the background mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the background mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the background mesh
	positionXYZ = glm::vec3(0.0f, 10.0f, -10.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	// White
	SetShaderColor(1, 1, 1, 1);

	// draw the background mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	/******************************************************************/
	/*** Below are the codes to draw all shapes needed to create    ***/
	/*** the ring stacker toy from the reference image.             ***/
	/******************************************************************/

	/******************************************************************/
	/*** Base of the Ring Stacker (Flat/Short Cylinder)             ***/
	/******************************************************************/
	// set the XYZ scale for the base mesh(flattened/short cylinder)
	scaleXYZ = glm::vec3(2.0f, 0.25f, 2.0f);

	// set the XYZ rotation for the base mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the base mesh
	positionXYZ = glm::vec3(10.0f, 0.0f, -1.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set and apply the wood texture
	SetShaderTexture("oakWood");

	// draw the base mesh 
	m_basicMeshes->DrawCylinderMesh();

	// Unbind the texture to prevent it from affecting other objects
	glBindTexture(GL_TEXTURE_2D, 0);
	/******************************************************************/


	/******************************************************************/
	/*** Vertical Rod (Holds the stackable rings)                   ***/
	/******************************************************************/
	// set the XYZ scale for the rod mesh (cylinder mesh)
	scaleXYZ = glm::vec3(0.2f, 5.2f, 0.2f);

	// set the XYZ rotation for the rod mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the rod mesh
	positionXYZ = glm::vec3(10.0f, 0.1f, -1.5f);

	// apply transformations for the rod mesh
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set and apply the wood texture
	SetShaderTexture("oakWood");

	// draw the rod mesh (Vertical rod for rings)
	m_basicMeshes->DrawCylinderMesh();

	// Unbind the texture to prevent it from affecting other objects
	glBindTexture(GL_TEXTURE_2D, 0);
	/******************************************************************/

	/******************************************************************/
	/*** Set needed transformations before drawing Ring 1           ***/
	/*** (Light-Blue).                                              ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	/******************************************************************/
	/*** Ring 1: Largest, Bottom-Most Light Blue Ring               ***/
	/******************************************************************/

	// set the XYZ scale for the light-blue ring mesh
	scaleXYZ = glm::vec3(2.0f, 2.0f, 2.0f);

	// set the XYZ rotation for the ring mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the ring mesh
	positionXYZ = glm::vec3(10.0f, 0.6f, -1.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set light blue plastic texture
	SetShaderTexture("ltbluePlastic");

	// draw the torus mesh (Ring 1 - Bottom, Light-Blue)
	m_basicMeshes->DrawTorusMesh();

	// Unbind the texture to prevent it from affecting other objects
	glBindTexture(GL_TEXTURE_2D, 0);
	/******************************************************************/


	/******************************************************************/
	/*** Set needed transformations before drawing Ring 2 (Blue). ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                      ***/
	/******************************************************************/

	/******************************************************************/
	/*** Ring 2: Slightly Smaller Blue Ring                         ***/
	/******************************************************************/

	// set the XYZ scale for the blue ring mesh
	scaleXYZ = glm::vec3(1.75f, 1.75f, 1.75f);

	// set the XYZ rotation for the ring mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the ring mesh
	positionXYZ = glm::vec3(10.0f, 1.7f, -1.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set blue plastic texture
	SetShaderTexture("bluePlastic");

	// draw the torus mesh (Ring 2 - Blue)
	m_basicMeshes->DrawTorusMesh();

	// Unbind the texture to prevent it from affecting other objects
	glBindTexture(GL_TEXTURE_2D, 0);
	/******************************************************************/


	/******************************************************************/
	/*** Set needed transformations before drawing Ring 3 (Pink).   ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	/******************************************************************/
	/*** Ring 3: Slightly Smaller Magenta Ring                         ***/
	/******************************************************************/

	//set the XYZ scale for the Magenta ring mesh
	scaleXYZ = glm::vec3(1.5f, 1.5f, 1.5f);

	// set the XYZ rotation for the ring mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the ring mesh
	positionXYZ = glm::vec3(10.0f, 2.65f, -1.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set magenta plastic texture
	SetShaderTexture("magentaPlastic");

	// draw the torus mesh (Ring 3 - Magenta)
	m_basicMeshes->DrawTorusMesh();

	// Unbind the texture to prevent it from affecting other objects
	glBindTexture(GL_TEXTURE_2D, 0);
	/******************************************************************/


	/******************************************************************/
	/*** Set needed transformations before drawing Ring 4 (Red).    ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	/******************************************************************/
	/*** Ring 4: Slightly Smaller Red Ring                          ***/
	/******************************************************************/

	// set the XYZ scale for the red ring mesh
	scaleXYZ = glm::vec3(1.25f, 1.25f, 1.25f);

	// set the XYZ rotation for the ring mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the ring mesh
	positionXYZ = glm::vec3(10.0f, 3.4f, -1.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set red plastic texture
	SetShaderTexture("redPlastic");

	// draw the torus mesh (Ring 4 - Red)
	m_basicMeshes->DrawTorusMesh();

	// Unbind the texture to prevent it from affecting other objects
	glBindTexture(GL_TEXTURE_2D, 0);
	/******************************************************************/


	/******************************************************************/
	/*** Set needed transformations before drawing Ring 5 (Yellow). ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	/******************************************************************/
	/*** Ring 5: Slightly Smaller Orange Ring                          ***/
	/******************************************************************/

	// set the XYZ scale for the Orange ring mesh
	scaleXYZ = glm::vec3(1.0f, 1.0f, 1.0f);

	// set the XYZ rotation for the ring mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the ring mesh
	positionXYZ = glm::vec3(10.0f, 4.05f, -1.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	// Yellow
	// SetShaderColor(1.0f, 0.78431f, 0.0f, 1.0f);

	// Set orange plastic texture
	SetShaderTexture("orangePlastic");

	// draw the torus mesh (Ring 5 - Yellow)
	m_basicMeshes->DrawTorusMesh();
	/******************************************************************/


	/******************************************************************/
	/*** Set needed transformations before drawing Ring 4 (Red).    ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	/******************************************************************/
	/*** Ring 6: Slightly Smaller Green Ring                        ***/
	/******************************************************************/
	// set the XYZ scale for the green ring mesh
	scaleXYZ = glm::vec3(0.75f, 0.75f, 0.75f);

	// set the XYZ rotation for the ring mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the ring mesh
	positionXYZ = glm::vec3(10.0f, 4.6f, -1.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set UV scaling	
	// SetTextureUVScale(0.1f, 0.1f);
	// Set green plastic texture
	SetShaderTexture("greenPlastic");

	// draw the torus mesh (Ring 6 - Green)
	m_basicMeshes->DrawExtraTorusMesh1();
	/******************************************************************/

	/******************************************************************/
	/*** Below are the codes to draw all shapes needed to create    ***/
	/*** the bead maze toy from the reference image.                ***/
	/******************************************************************/

	/******************************************************************/
	/*** Set needed transformations before drawing Ring 5 (Yellow). ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	/******************************************************************/
	/*** Base of the bead maze (rectangular shaped box)             ***/
	/******************************************************************/

	// set the XYZ scale for base mesh
	scaleXYZ = glm::vec3(1.0f, 0.75f, 10.0f);

	// set the XYZ rotation for the base mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the base mesh
	positionXYZ = glm::vec3(0.0f, 0.35f, -3.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set and apply the wood texture
	SetShaderTexture("oakWood");

	// draw the box mesh 
	m_basicMeshes->DrawBoxMesh();

	// Unbind the texture to prevent it from affecting other objects
	glBindTexture(GL_TEXTURE_2D, 0);
	/******************************************************************/

	/******************************************************************/
	/*** Set needed transformations before drawing the rod          ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	/******************************************************************/
	/*** Taller, vertical rod on the right of the bead maze         ***/
	/*** (long, skinny cylinder)                                    ***/
	/******************************************************************/

	// set the XYZ scale for rod mesh
	scaleXYZ = glm::vec3(0.05f, 5.0f, 0.05f);

	// set the XYZ rotation for the rod mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the rod mesh
	positionXYZ = glm::vec3(4.25f, 0.75f, -3.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	
	// Set UV scaling	
	SetTextureUVScale(0.1f, 0.1f);
	// Set and apply the metal texture
	SetShaderTexture("steelTexture");

	// draw the rod mesh 
	m_basicMeshes->DrawCylinderMesh();
	
	// Unbind the texture to prevent it from affecting other objects
	glBindTexture(GL_TEXTURE_2D, 0);
	/******************************************************************/

	/******************************************************************/
	/*** Set needed transformations before drawing the rod.         ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	/******************************************************************/
	/*** Taller, vertical rod on the left of the bead maze          ***/
	/*** (long, skinny cylinder)                                    ***/
	/******************************************************************/

	// set the XYZ scale for rod mesh
	scaleXYZ = glm::vec3(0.05f, 5.0f, 0.05f);

	// set the XYZ rotation for the rod mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the rod mesh
	positionXYZ = glm::vec3(-4.25f, 0.75f, -3.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set UV scaling	
	SetTextureUVScale(0.1f, 0.1f);
	// Set and apply the metal texture
	SetShaderTexture("steelTexture");

	// draw the rod mesh 
	m_basicMeshes->DrawCylinderMesh();

	// Unbind the texture to prevent it from affecting other objects
	glBindTexture(GL_TEXTURE_2D, 0);
	/******************************************************************/

	/******************************************************************/
	/*** Set needed transformations before drawing the rod.         ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	/******************************************************************/
	/*** Longer, horizontal rod of the bead maze                    ***/
	/*** (long, skinny cylinder)                                    ***/
	/******************************************************************/

	// set the XYZ scale for rod mesh
	scaleXYZ = glm::vec3(0.05f, 8.1f, 0.05f);

	// set the XYZ rotation for the rod mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the rod mesh
	positionXYZ = glm::vec3(4.05f, 5.95f, -3.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set UV scaling	
	SetTextureUVScale(0.1f, 0.1f);
	// Set and apply the metal texture
	SetShaderTexture("steelTexture");

	// draw the rod mesh 
	m_basicMeshes->DrawCylinderMesh();

	// Unbind the texture to prevent it from affecting other objects
	glBindTexture(GL_TEXTURE_2D, 0);
	/******************************************************************/

	/******************************************************************/
	/*** Set needed transformations before drawing the rod.         ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	/******************************************************************/
	/*** Curve where the longer, horizontal rod and taller right    ***/
	/*** vertical rod of the bead maze meet. (quarter torus)        ***/
	/******************************************************************/

	// set the XYZ scale for rod-curve mesh
	scaleXYZ = glm::vec3(0.2f, 0.2f, 0.175f);

	// set the XYZ rotation for the rod-curve mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the rod-curve mesh
	positionXYZ = glm::vec3(4.05f, 5.75f, -3.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set UV scaling	
	SetTextureUVScale(0.1f, 0.1f);
	// Set and apply the metal texture
	SetShaderTexture("steelTexture");

	// draw the rod-curve mesh 
	m_basicMeshes->DrawQuarterTorusMesh(0.2f);
	
	// Unbind the texture to prevent it from affecting other objects
	glBindTexture(GL_TEXTURE_2D, 0);
	/******************************************************************/

	/******************************************************************/
	/*** Set needed transformations before drawing the rod.         ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	/******************************************************************/
	/*** Curve where longer, horizontal rod and taller left         ***/
	/*** vertical rod of the bead maze meet. (quarter torus)        ***/
	/******************************************************************/
	
	// set the XYZ scale for rod-curve mesh
	scaleXYZ = glm::vec3(0.2f, 0.2f, 0.175f);

	// set the XYZ rotation for the rod-curve mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the rod-curve mesh
	positionXYZ = glm::vec3(-4.05f, 5.75f, -3.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set UV scaling	
	SetTextureUVScale(0.1f, 0.1f);
	// Set and apply the metal texture
	SetShaderTexture("steelTexture");

	// draw the rod-curve mesh 
	m_basicMeshes->DrawQuarterTorusMesh(0.2f);

	// Unbind the texture to prevent it from affecting other objects
	glBindTexture(GL_TEXTURE_2D, 0);
	/******************************************************************/


	/******************************************************************/
	/*** Set needed transformations before drawing the rod.         ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	/******************************************************************/
	/*** Shorter, vertical rod on the right of the bead maze        ***/
	/*** (long, skinny cylinder)                                    ***/
	/******************************************************************/
	
	// set the XYZ scale for rod mesh
	scaleXYZ = glm::vec3(0.05f, 3.0f, 0.05f);

	// set the XYZ rotation for the rod mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the rod mesh
	positionXYZ = glm::vec3(2.5f, 0.75f, -3.5f);	

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set UV scaling	
	SetTextureUVScale(0.1f, 0.1f);
	// Set and apply the metal texture
	SetShaderTexture("steelTexture");

	// draw the rod mesh
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	/******************************************************************/
	/*** Set needed transformations before drawing the rod.         ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	/******************************************************************/
	/*** Shorter, vertical rod on the left of the bead maze         ***/
	/*** (long, skinny cylinder)                                    ***/
	/******************************************************************/
	
	// set the XYZ scale for rod mesh
	scaleXYZ = glm::vec3(0.05f, 3.0f, 0.05f);

	// set the XYZ rotation for the rod mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the rod mesh
	positionXYZ = glm::vec3(-2.5f, 0.75f, -3.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	
	// Set UV scaling	
	SetTextureUVScale(0.1f, 0.1f);
	// Set and apply the metal texture
	SetShaderTexture("steelTexture");

	// draw the rod mesh
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	/******************************************************************/
	/*** Set needed transformations before drawing the rod.         ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	/******************************************************************/
	/*** Shorter, horizontal rod of the bead maze                   ***/
	/*** (long, skinny cylinder)                                    ***/
	/******************************************************************/
	
	// set the XYZ scale for rod mesh
	scaleXYZ = glm::vec3(0.05f, 4.75f, 0.05f);

	// set the XYZ rotation for the rod mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the rod mesh
	positionXYZ = glm::vec3(2.375f, 3.95f, -3.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	// SetShaderColor(0.2901f, 0.2862f, 0.2823f, 1.0f);

	// Set UV scaling	
	SetTextureUVScale(0.1f, 0.1f);
	// Set and apply the metal texture
	SetShaderTexture("steelTexture");

	// draw the rod mesh
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	/******************************************************************/
	/*** Set needed transformations before drawing the rod.         ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	/******************************************************************/
	/*** Curve where shorter, horizontal rod and shorter right      ***/
	/*** vertical rod of the bead maze meet. (quarter torus)        ***/
	/******************************************************************/

	// set the XYZ scale for rod-curve mesh
	scaleXYZ = glm::vec3(0.2f, 0.2f, 0.175f);

	// set the XYZ rotation for the rod-curve mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the rod-curve mesh
	positionXYZ = glm::vec3(2.3f, 3.75f, -3.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set UV scaling	
	SetTextureUVScale(0.1f, 0.1f);
	// Set and apply the metal texture
	SetShaderTexture("steelTexture");

	// draw the rod-curve mesh 
	m_basicMeshes->DrawQuarterTorusMesh(0.2f);

	// Unbind the texture to prevent it from affecting other objects
	glBindTexture(GL_TEXTURE_2D, 0);
	/******************************************************************/

	/******************************************************************/
	/*** Set needed transformations before drawing the rod.         ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	/******************************************************************/
	/*** Curve where shorter, horizontal rod and shorter left       ***/
	/*** vertical rod of the bead maze meet. (quarter torus)        ***/
	/******************************************************************/

	// set the XYZ scale for rod-curve mesh
	scaleXYZ = glm::vec3(0.2f, 0.2f, 0.175f);

	// set the XYZ rotation for the rod-curve mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the rod-curve mesh
	positionXYZ = glm::vec3(-2.3f, 3.75f, -3.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set UV scaling	
	SetTextureUVScale(0.1f, 0.1f);
	// Set and apply the metal texture
	SetShaderTexture("steelTexture");

	// draw the rod-curve mesh 
	m_basicMeshes->DrawQuarterTorusMesh(0.2f);

	// Unbind the texture to prevent it from affecting other objects
	glBindTexture(GL_TEXTURE_2D, 0);
	/******************************************************************/

	/******************************************************************/
	/*** Set needed transformations before drawing the rod.         ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	/******************************************************************/
	/*** Bead #1 on the bead maze, bottom right, blue bead          ***/
	/******************************************************************/

	// set the XYZ scale for the bead mesh
	scaleXYZ = glm::vec3(0.75f, 0.75f, 0.75f);

	// set the XYZ rotation for the bead mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the bead mesh
	positionXYZ = glm::vec3(4.25f, 1.5f, -3.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set UV scaling
	 SetTextureUVScale(0.5f, 0.5f);
	// Set and apply the blue plastic texture
	SetShaderTexture("bluePlastic");

	// draw the bead mesh 
	m_basicMeshes->DrawSphereMesh();

	// Unbind the texture to prevent it from affecting other objects
	glBindTexture(GL_TEXTURE_2D, 0);
	/******************************************************************/

	/******************************************************************/
	/*** Set needed transformations before drawing the rod.         ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	/******************************************************************/
	/*** Bead #2 on the bead maze, second on right, light blue bead ***/
	/******************************************************************/

	// set the XYZ scale for the bead mesh
	scaleXYZ = glm::vec3(0.75f, 0.75f, 0.75f);

	// set the XYZ rotation for the bead mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the bead mesh
	positionXYZ = glm::vec3(4.25f, 3.0f, -3.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set UV scaling	
	SetTextureUVScale(0.5f, 0.5f); 
	// Set and apply the light bkue plastic texture
	SetShaderTexture("ltbluePlastic");

	// draw the bead mesh 
	m_basicMeshes->DrawSphereMesh();

	// Unbind the texture to prevent it from affecting other objects
	glBindTexture(GL_TEXTURE_2D, 0);
	/******************************************************************/

	/******************************************************************/
	/*** Set needed transformations before drawing the rod.         ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	/******************************************************************/
	/*** Bead #3 on the bead maze, third on right, green bead       ***/
	/******************************************************************/

	// set the XYZ scale for the bead mesh
	scaleXYZ = glm::vec3(0.75f, 0.75f, 0.75f);

	// set the XYZ rotation for the bead mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the bead mesh
	positionXYZ = glm::vec3(4.25f, 4.5f, -3.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set UV scaling
	SetTextureUVScale(0.5f, 0.5f);
	// Set and apply the green plastic texture
	SetShaderTexture("greenPlastic");

	// draw the bead mesh 
	m_basicMeshes->DrawSphereMesh();

	// Unbind the texture to prevent it from affecting other objects
	glBindTexture(GL_TEXTURE_2D, 0);
	/******************************************************************/

	/******************************************************************/
	/*** Set needed transformations before drawing the rod.         ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	/******************************************************************/
	/*** Bead #4 on the bead maze, bottom left, red bead            ***/
	/******************************************************************/

	// set the XYZ scale for the bead mesh
	scaleXYZ = glm::vec3(0.75f, 0.75f, 0.75f);

	// set the XYZ rotation for the bead mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the bead mesh
	positionXYZ = glm::vec3(-4.25f, 1.5f, -3.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set UV scaling
	SetTextureUVScale(0.5f, 0.5f);
	// Set and apply the red plastic texture
	SetShaderTexture("redPlastic");

	// draw the bead mesh 
	m_basicMeshes->DrawSphereMesh();

	// Unbind the texture to prevent it from affecting other objects
	glBindTexture(GL_TEXTURE_2D, 0);
	/******************************************************************/

	/******************************************************************/
	/*** Set needed transformations before drawing the rod.         ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	/******************************************************************/
	/*** Bead #5 on the bead maze, second bead on left, orange bead ***/
	/******************************************************************/

	// set the XYZ scale for the bead mesh
	scaleXYZ = glm::vec3(0.75f, 0.75f, 0.75f);

	// set the XYZ rotation for the bead mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the bead mesh
	positionXYZ = glm::vec3(-4.25f, 3.0f, -3.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set UV scaling
	SetTextureUVScale(0.5f, 0.5f);
	// Set and apply the orange plastic texture
	SetShaderTexture("orangePlastic");

	// draw the bead mesh 
	m_basicMeshes->DrawSphereMesh();

	// Unbind the texture to prevent it from affecting other objects
	glBindTexture(GL_TEXTURE_2D, 0);
	/******************************************************************/

	/******************************************************************/
	/*** Set needed transformations before drawing the rod.         ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	/******************************************************************/
	/*** Bead #6 on the bead maze, bottom right, second rod,        ***/
	/*** magenta bead                                               ***/
	/******************************************************************/

	// set the XYZ scale for the bead mesh
	scaleXYZ = glm::vec3(0.75f, 0.75f, 0.75f);

	// set the XYZ rotation for the bead mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the bead mesh
	positionXYZ = glm::vec3(2.375f, 1.5f, -3.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set UV scaling
	SetTextureUVScale(0.5f, 0.5f);
	// Set and apply the magenta plastic texture
	SetShaderTexture("magentaPlastic");

	// draw the bead mesh 
	m_basicMeshes->DrawSphereMesh();

	// Unbind the texture to prevent it from affecting other objects
	glBindTexture(GL_TEXTURE_2D, 0);
	/******************************************************************/

	/******************************************************************/
	/*** Set needed transformations before drawing the rod.         ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	/******************************************************************/
	/*** Bead #7 on the bead maze, second on right, second rod,     ***/
	/*** red bead                                                   ***/
	/******************************************************************/

	// set the XYZ scale for the bead mesh
	scaleXYZ = glm::vec3(0.75f, 0.75f, 0.75f);

	// set the XYZ rotation for the bead mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the bead mesh
	positionXYZ = glm::vec3(2.375f, 3.0f, -3.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set UV scaling
	SetTextureUVScale(0.5f, 0.5f);
	// Set and apply the red plastic texture
	SetShaderTexture("redPlastic");

	// draw the bead mesh 
	m_basicMeshes->DrawSphereMesh();

	// Unbind the texture to prevent it from affecting other objects
	glBindTexture(GL_TEXTURE_2D, 0);
	/******************************************************************/

	/******************************************************************/
	/*** Set needed transformations before drawing the rod.         ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	/******************************************************************/
	/*** Bead #8 on the bead maze, bottom right, second rod,        ***/
	/*** magenta bead                                               ***/
	/******************************************************************/

	// set the XYZ scale for the bead mesh
	scaleXYZ = glm::vec3(0.75f, 0.75f, 0.75f);

	// set the XYZ rotation for the bead mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the bead mesh
	positionXYZ = glm::vec3(0.75f, 3.95f, -3.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set UV scaling
	SetTextureUVScale(0.5f, 0.5f);
	// Set and apply the orange plastic texture
	SetShaderTexture("orangePlastic");

	// draw the bead mesh 
	m_basicMeshes->DrawSphereMesh();

	// Unbind the texture to prevent it from affecting other objects
	glBindTexture(GL_TEXTURE_2D, 0);
	/******************************************************************/

	/******************************************************************/
	/*** Set needed transformations before drawing the rod.         ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	/******************************************************************/
	/*** Bead #9 on the bead maze, bottom left, second rod,        ***/
	/*** light blue bead                                               ***/
	/******************************************************************/

	// set the XYZ scale for the bead mesh
	scaleXYZ = glm::vec3(0.75f, 0.75f, 0.75f);

	// set the XYZ rotation for the bead mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the bead mesh
	positionXYZ = glm::vec3(-0.75f, 3.95f, -3.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set UV scaling
	SetTextureUVScale(0.5f, 0.5f);
	// Set and apply the green plastic texture
	SetShaderTexture("greenPlastic");

	// draw the bead mesh 
	m_basicMeshes->DrawSphereMesh();

	// Unbind the texture to prevent it from affecting other objects
	glBindTexture(GL_TEXTURE_2D, 0);
	/******************************************************************/

	/******************************************************************/
	/*** Set needed transformations before drawing the rod.         ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	/******************************************************************/
	/*** Bead #6 on the bead maze, bottom right, second rod,        ***/
	/*** magenta bead                                               ***/
	/******************************************************************/

	// set the XYZ scale for the bead mesh
	scaleXYZ = glm::vec3(0.75f, 0.75f, 0.75f);

	// set the XYZ rotation for the bead mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the bead mesh
	positionXYZ = glm::vec3(-2.375f, 1.5f, -3.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set UV scaling
	SetTextureUVScale(0.5f, 0.5f);
	// Set and apply the light blue plastic texture
	SetShaderTexture("ltbluePlastic");

	// draw the bead mesh 
	m_basicMeshes->DrawSphereMesh();

	// Unbind the texture to prevent it from affecting other objects
	glBindTexture(GL_TEXTURE_2D, 0);
	/******************************************************************/

	/******************************************************************/
	/*** Set needed transformations before drawing the rod.         ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	/******************************************************************/
	/*** Letter block A (box, double textures)                      ***/
	/******************************************************************/

	// set the XYZ scale for the block mesh
	scaleXYZ = glm::vec3(2.0f, 2.0f, 2.0f);

	// set the XYZ rotation for the block mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 15.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the block mesh
	positionXYZ = glm::vec3(-0.75f, 1.0f, 0.75f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set UV scaling
	SetTextureUVScale(1.0f, 1.0f);
	// Set and apply the ash wood texture
	SetShaderTexture("ashWood");

	// draw the block mesh 
	m_basicMeshes->DrawBoxMesh();

	// set the XYZ scale for the block mesh
	scaleXYZ = glm::vec3(2.01f, 2.01f, 2.01f);

	// set the XYZ rotation for the overlay block mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 15.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the overlay block mesh
	positionXYZ = glm::vec3(-0.7501f, 1.01f, 0.7501f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	
	// Set UV scaling
	SetTextureUVScale(1.0f, 1.0f);
	//SetShaderTexture("letterA");	
	// Overlay the letter texture
	SetShaderTexture("letterA");

	// draw the overlay block mesh 
	m_basicMeshes->DrawBoxMesh();

	glPolygonOffset(-1.0f, -1.0f);  // Prevent z-fighting 

	// Unbind the texture to prevent it from affecting other objects
	glBindTexture(GL_TEXTURE_2D, 0);
	/******************************************************************/

	/******************************************************************/
	/*** Set needed transformations before drawing the rod.         ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                       ***/
	/******************************************************************/

	/******************************************************************/
	/*** Letter block B (box, double textures)                      ***/
	/******************************************************************/

	// set the XYZ scale for the block mesh
	scaleXYZ = glm::vec3(2.0f, 2.0f, 2.0f);

	// set the XYZ rotation for the block mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the block mesh
	positionXYZ = glm::vec3(2.0f, 1.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set UV scaling
	SetTextureUVScale(1.0f, 1.0f);
	// Draw the base block with wood texture
	SetShaderTexture("ashWood");

	// draw the block mesh 
	m_basicMeshes->DrawBoxMesh();

	// set the XYZ scale for the overlay block mesh
	scaleXYZ = glm::vec3(2.01f, 2.01f, 2.01f);

	// set the XYZ rotation for the overlay block mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the overlay block mesh
	positionXYZ = glm::vec3(2.01f, 1.01f, 0.01f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set UV scaling
	SetTextureUVScale(1.0f, 1.0f);
	//SetShaderTexture("letterB");	
	// Overlay the letter texture
	SetShaderTexture("letterB");

	// draw the overlay block mesh 
	m_basicMeshes->DrawBoxMesh();

	glPolygonOffset(-1.0f, -1.0f);  // Prevent z-fighting 

	// Unbind the texture to prevent it from affecting other objects
	glBindTexture(GL_TEXTURE_2D, 0);
	/******************************************************************/

	/******************************************************************/
	/*** Letter block C (box, double textures)                      ***/
	/******************************************************************/

	// set the XYZ scale for the block mesh
	scaleXYZ = glm::vec3(2.0f, 2.0f, 2.0f);

	// set the XYZ rotation for the block mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 25.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the block mesh
	positionXYZ = glm::vec3(0.75f, 3.0f, 0.75f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set UV scaling
	SetTextureUVScale(1.0f, 1.0f);
	// Draw the base block with wood texture
	SetShaderTexture("ashWood");
	
	// Draw the first block mesh
	m_basicMeshes->DrawBoxMesh();


	// set the XYZ scale for the overlay block mesh
	scaleXYZ = glm::vec3(2.01f, 2.01f, 2.01f);

	// set the XYZ rotation for the overlay block mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 25.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the overlay block mesh
	positionXYZ = glm::vec3(0.7501f, 3.01f, 0.7501f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	//SetShaderTexture("letterC");	
	// Overlay the letter texture
	SetShaderTexture("letterC");
	// Draw the overlay block mesh with letter texture
	m_basicMeshes->DrawBoxMesh();

	glPolygonOffset(-1.0f, -1.0f);  // Prevent z-fighting 

	// Unbind the texture to prevent it from affecting other objects
	glBindTexture(GL_TEXTURE_2D, 0);
	/******************************************************************/

}
