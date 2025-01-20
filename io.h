#pragma once

namespace vkengine
{
	namespace io
	{
		std::vector<uint8_t> ReadFile(const std::string& filename);
		
		void WriteFile(const std::string& filename, void* data, size_t dataSize);

		void DeleteFile(const std::string& filename);

		std::string GetBaseDir(const std::string& filepath);

		bool FileExists(const std::string& abs_filename);
	}
}