
#include "CommandBufferGL.h"
#include "MaterialGL.h"
#include "Shader.h"
#include "graphics/VertexBuffer.h"
#include "graphics/opengl/Program.h"
#include "graphics/opengl/RendererGL.h"
#include "graphics/opengl/TextureGL.h"
#include "graphics/opengl/UniformBuffer.h"
#include "graphics/opengl/VertexBufferGL.h"

using namespace Graphics::OGL;

void CommandList::AddDrawCmd(Graphics::MeshObject *mesh, Graphics::Material *material, Graphics::InstanceBuffer *inst)
{
	assert(!m_executing && "Attempt to append to a command list while it's being executed!");
	OGL::Material *mat = static_cast<OGL::Material *>(material);

	DrawCmd cmd;
	cmd.mesh = static_cast<OGL::MeshObject *>(mesh);
	cmd.program = mat->EvaluateVariant();
	cmd.shader = mat->GetShader();
	cmd.inst = static_cast<OGL::InstanceBuffer *>(inst);
	cmd.renderStateHash = mat->m_renderStateHash;
	cmd.drawData = SetupMaterialData(mat);

	m_drawCmds.emplace_back(std::move(cmd));
}

void CommandList::Reset()
{
	assert(!m_executing && "Attempt to reset a command list while it's being processed!");

	for (auto &bucket : m_dataBuckets)
		bucket.used = 0;

	m_drawCmds.clear();
}

template <size_t I>
size_t align(size_t t)
{
	return (t + (I - 1)) & ~(I - 1);
}

char *CommandList::AllocDrawData(const Shader *shader)
{
	size_t constantSize = align<8>(shader->GetConstantStorageSize());
	size_t bufferSize = align<8>(shader->GetNumBufferBindings() * sizeof(UniformBufferBinding));
	size_t textureSize = align<8>(shader->GetNumTextureBindings() * sizeof(Texture *));
	size_t totalSize = constantSize + bufferSize + textureSize;

	char *alloc = nullptr;
	for (auto &bucket : m_dataBuckets) {
		if ((alloc = bucket.alloc(totalSize)))
			break;
	}

	if (!alloc) {
		DataBucket bucket{};
		bucket.data.reset(new char[BUCKET_SIZE]);
		alloc = bucket.alloc(totalSize);
		m_dataBuckets.emplace_back(std::move(bucket));
	}

	assert(alloc != nullptr);
	memset(alloc, '\0', totalSize);
	return alloc;
}

UniformBufferBinding *CommandList::getBufferBindings(const Shader *shader, char *data)
{
	size_t constantSize = align<8>(shader->GetConstantStorageSize());
	return reinterpret_cast<UniformBufferBinding *>(data + constantSize);
}

TextureGL **CommandList::getTextureBindings(const Shader *shader, char *data)
{
	size_t constantSize = align<8>(shader->GetConstantStorageSize());
	size_t bufferSize = align<8>(shader->GetNumBufferBindings() * sizeof(UniformBufferBinding));
	return reinterpret_cast<TextureGL **>(data + constantSize + bufferSize);
}

char *CommandList::SetupMaterialData(OGL::Material *mat)
{
	PROFILE_SCOPED()
	mat->UpdateDrawData();
	const Shader *s = mat->GetShader();

	char *alloc = AllocDrawData(s);
	memcpy(alloc, mat->m_pushConstants.get(), s->GetConstantStorageSize());

	UniformBufferBinding *buffers = getBufferBindings(s, alloc);
	for (size_t index = 0; index < s->GetNumBufferBindings(); index++) {
		auto &info = mat->m_bufferBindings[index];
		buffers[index].buffer = info.buffer.Get();
		buffers[index].offset = info.offset;
		buffers[index].size = info.size;
	}

	TextureGL **textures = getTextureBindings(s, alloc);
	for (size_t index = 0; index < s->GetNumTextureBindings(); index++) {
		textures[index] = static_cast<TextureGL *>(mat->m_textureBindings[index]);
	}

	return alloc;
}

void CommandList::ApplyDrawData(const DrawCmd &cmd) const
{
	PROFILE_SCOPED();
	cmd.program->Use();

	UniformBufferBinding *buffers = getBufferBindings(cmd.shader, cmd.drawData);
	for (auto &info : cmd.shader->GetBufferBindings()) {
		UniformBufferBinding &bind = buffers[info.index];
		if (bind.buffer)
			bind.buffer->BindRange(info.binding, bind.offset, bind.size);
	}

	TextureGL **textures = getTextureBindings(cmd.shader, cmd.drawData);
	for (auto &info : cmd.shader->GetTextureBindings()) {
		glActiveTexture(GL_TEXTURE0 + info.binding);
		if (textures[info.index])
			textures[info.index]->Bind();
		else
			glBindTexture(GL_TEXTURE_2D, 0);
	}

	for (auto &info : cmd.shader->GetPushConstantBindings()) {
		GLuint location = cmd.program->GetConstantLocation(info.binding);
		if (location == GL_INVALID_INDEX)
			continue;

		Uniform setter(location);
		switch (info.format) {
		case ConstantDataFormat::DATA_FORMAT_INT:
			setter.Set(*reinterpret_cast<int *>(cmd.drawData + info.offset));
			break;
		case ConstantDataFormat::DATA_FORMAT_FLOAT:
			setter.Set(*reinterpret_cast<float *>(cmd.drawData + info.offset));
			break;
		case ConstantDataFormat::DATA_FORMAT_FLOAT3:
			setter.Set(*reinterpret_cast<vector3f *>(cmd.drawData + info.offset));
			break;
		case ConstantDataFormat::DATA_FORMAT_FLOAT4:
			setter.Set(*reinterpret_cast<Color4f *>(cmd.drawData + info.offset));
			break;
		case ConstantDataFormat::DATA_FORMAT_MAT3:
			setter.Set(*reinterpret_cast<matrix3x3f *>(cmd.drawData + info.offset));
			break;
		case ConstantDataFormat::DATA_FORMAT_MAT4:
			setter.Set(*reinterpret_cast<matrix4x4f *>(cmd.drawData + info.offset));
			break;
		default:
			assert(false);
			break;
		}
	}
}

void CommandList::CleanupDrawData(const DrawCmd &cmd) const
{
	// Push constants (glUniforms) don't need to be unbound
	// Uniform buffers also don't need to be unbound

	// Unbinding textures is probably also not needed, (and not performant)
	// but included here to ensure we don't have any state leakage
	TextureGL **textures = getTextureBindings(cmd.shader, cmd.drawData);
	for (auto &info : cmd.shader->GetTextureBindings()) {
		if (textures[info.index]) {
			glActiveTexture(GL_TEXTURE0 + info.binding);
			textures[info.index]->Unbind();
		}
	}
}
