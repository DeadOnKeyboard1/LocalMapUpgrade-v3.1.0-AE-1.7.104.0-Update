#include "ShaderManager.h"
#include "utils/Logger.h"

#include <d3dcompiler.h>

namespace LMU
{
	static constexpr const char* pixelShaderSrc =
	{
#include "ISLocalMap.hlsl.h"
	};

	ShaderManager::ShaderManager()
	{
		auto* imageSpaceManager = RE::ImageSpaceManager::GetSingleton();
		if (!imageSpaceManager) {
			logger::critical("Could not initialize local-map shaders: ImageSpaceManager is unavailable");
			return;
		}

		const std::uint32_t isLocalMapIndex = RE::ImageSpaceManager::ISLocalMap;
		auto* localMapShaderEffect = imageSpaceManager->effects[isLocalMapIndex];
		auto* localMapShader = skyrim_cast<RE::BSImagespaceShader*>(localMapShaderEffect);
		if (!localMapShader || localMapShader->pixelShaders.empty()) {
			logger::critical("Could not find the local-map pixel shader");
			return;
		}

		localMapPixelShader = *localMapShader->pixelShaders.begin();
		if (!localMapPixelShader) {
			logger::critical("Local-map pixel-shader slot is null");
			return;
		}

		squaredShaders.blackNWhite.fogOfWar = CompilePixelShader(pixelShaderSrc);
		squaredShaders.blackNWhite.noFogOfWar = CompilePixelShader(pixelShaderSrc, { "NO_FOG_OF_WAR" });
		squaredShaders.color.fogOfWar = CompilePixelShader(pixelShaderSrc, { "COLOR" });
		squaredShaders.color.noFogOfWar = CompilePixelShader(pixelShaderSrc, { "COLOR", "NO_FOG_OF_WAR" });
		roundShaders.blackNWhite.fogOfWar = CompilePixelShader(pixelShaderSrc, { "ROUND" });
		roundShaders.blackNWhite.noFogOfWar = CompilePixelShader(pixelShaderSrc, { "ROUND", "NO_FOG_OF_WAR" });
		roundShaders.color.fogOfWar = CompilePixelShader(pixelShaderSrc, { "ROUND", "COLOR" });
		roundShaders.color.noFogOfWar = CompilePixelShader(pixelShaderSrc, { "ROUND", "COLOR", "NO_FOG_OF_WAR" });

		isFogOfWarEnabled = settings::mapmenu::localMapFogOfWar;
		SetPixelShaderProperties(shape, style);
	}

	ShaderManager::~ShaderManager()
	{
		ReleaseShaderGroup(squaredShaders);
		ReleaseShaderGroup(roundShaders);
	}

	void ShaderManager::ReleaseShaderGroup(PixelShaderGroup& a_group)
	{
		auto release = [](REX::W32::ID3D11PixelShader*& a_shader) {
			if (a_shader) {
				a_shader->Release();
				a_shader = nullptr;
			}
		};
		release(a_group.blackNWhite.fogOfWar);
		release(a_group.blackNWhite.noFogOfWar);
		release(a_group.color.fogOfWar);
		release(a_group.color.noFogOfWar);
	}

	REX::W32::ID3D11PixelShader* ShaderManager::CompilePixelShader(const char* a_source,
		const std::vector<const char*>& a_defineNames)
	{
		if (!a_source || !*a_source) {
			logger::critical("Cannot compile an empty local-map pixel shader");
			return nullptr;
		}

		static constexpr std::uint32_t compileFlags =
			D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;
		std::vector<D3D_SHADER_MACRO> macros;
		macros.reserve(a_defineNames.size() + 1);
		for (const char* name : a_defineNames) {
			if (name && *name) {
				macros.push_back({ name, "" });
			}
		}
		macros.push_back({ nullptr, nullptr });

		ID3DBlob* shaderBlob = nullptr;
		ID3DBlob* errorBlob = nullptr;
		const HRESULT compileResult = D3DCompile(a_source, std::strlen(a_source), nullptr, macros.data(), nullptr,
			"main", "ps_5_0", compileFlags, 0, &shaderBlob, &errorBlob);
		if (FAILED(compileResult) || !shaderBlob) {
			logger::critical("Pixel shader failed to compile (HRESULT=0x{:08X})",
				static_cast<std::uint32_t>(compileResult));
			if (errorBlob) {
				logger::critical("{}", static_cast<const char*>(errorBlob->GetBufferPointer()));
				errorBlob->Release();
			}
			if (shaderBlob) {
				shaderBlob->Release();
			}
			return nullptr;
		}
		if (errorBlob) {
			errorBlob->Release();
		}

		auto* renderer = RE::BSGraphics::Renderer::GetSingleton();
		auto* device = renderer ? renderer->GetRuntimeData().forwarder : nullptr;
		if (!device) {
			logger::critical("Failed to create local-map pixel shader: D3D11 device unavailable");
			shaderBlob->Release();
			return nullptr;
		}

		REX::W32::ID3D11PixelShader* pixelShader = nullptr;
		const auto createResult = device->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(),
			nullptr, &pixelShader);
		shaderBlob->Release();
		if (FAILED(createResult) || !pixelShader) {
			logger::critical("Failed to create local-map pixel shader (HRESULT=0x{:08X})",
				static_cast<std::uint32_t>(createResult));
			return nullptr;
		}
		return pixelShader;
	}

	void ShaderManager::ToggleFogOfWarLocalMapShader()
	{
		isFogOfWarEnabled = !isFogOfWarEnabled;
		SetPixelShaderProperties(shape, style);
		if (RE::ConsoleLog::IsConsoleMode()) {
			if (auto* console = RE::ConsoleLog::GetSingleton()) {
				console->Print("Fog of war - %s.", isFogOfWarEnabled ? "ENABLED" : "DISABLED");
			}
		}
	}

	void ShaderManager::ApplySettings()
	{
		isFogOfWarEnabled = settings::mapmenu::localMapFogOfWar;
		style = settings::mapmenu::localMapColor ?
			PixelShaderProperty::Style::kColor : PixelShaderProperty::Style::kBlackNWhite;
		SetPixelShaderProperties(shape, style);
	}

	void ShaderManager::SetPixelShaderProperties(PixelShaderProperty::Shape a_shape, PixelShaderProperty::Style a_style)
	{
		auto* self = singleton;
		if (!self) {
			return;
		}

		PixelShaderGroup& shapeShaders = a_shape == PixelShaderProperty::Shape::kRound ? roundShaders : squaredShaders;
		PixelShaderGroup::FogOfWarGroup& candidates = a_style == PixelShaderProperty::Style::kColor ?
			shapeShaders.color : shapeShaders.blackNWhite;
		auto* shader = self->isFogOfWarEnabled ? candidates.fogOfWar : candidates.noFogOfWar;
		if (localMapPixelShader && shader) {
			localMapPixelShader->shader = shader;
		}
		self->shape = a_shape;
		self->style = a_style;
	}

	void ShaderManager::GetPixelShaderProperties(PixelShaderProperty::Shape& a_shape, PixelShaderProperty::Style& a_style)
	{
		if (singleton) {
			a_shape = singleton->shape;
			a_style = singleton->style;
		}
	}
}
