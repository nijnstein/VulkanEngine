#include "defines.h"

namespace vkengine
{
	namespace io
	{
		void WriteFile(const std::string& filename, void* data, size_t dataSize)
		{
			std::ofstream file(filename, std::ios::trunc | std::ios::binary);
			file.write((char*)data, dataSize); 
			file.close(); 
		}

		std::vector<uint8_t> ReadFile(const std::string& filename)
		{
			std::ifstream file(filename, std::ios::ate | std::ios::binary);

			if (!file.is_open())
			{
				throw std::runtime_error("failed to open file!");
			}

			size_t fileSize = (size_t)file.tellg();
			std::vector<uint8_t> buffer(fileSize);

			file.seekg(0);
			file.read((char*)buffer.data(), fileSize);
			file.close();

			return buffer;
		}

		void DeleteFile(const std::string& filename)
		{
			std::remove(filename.c_str()); 
		}

		std::string GetBaseDir(const std::string& filepath)
		{
			if (filepath.find_last_of("/\\") != std::string::npos)
			{
				return filepath.substr(0, filepath.find_last_of("/\\"));
			}
			return "";
		}

		bool FileExists(const std::string& abs_filename)
		{
			std::ifstream f(abs_filename.c_str());
			return f.good();
		}
	}
}