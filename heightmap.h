#pragma once

namespace vkengine
{
	class HeightMap
	{
		bool ownImage;
		ImageRaw image; 

	public:

		HeightMap()
		{
			ownImage = false;			
		}

		HeightMap(ImageRaw raw, bool ownImage = true)
		{
			setImage(raw, ownImage); 
		}

		~HeightMap()
		{
			if (ownImage && image.data)
			{
				image.deallocate(); 
			}
		}

		void setImage(ImageRaw raw, bool ownThisImage = true)
		{
			image = raw;
			ownImage = ownThisImage;
		}

		float get_height(const UINT x, const UINT y)
		{
			UINT idx = image.depth * (x + y * image.width); 
			BYTE* p = &image.data[idx]; 
			UINT v = (p[0] + p[1] + p[2]); 
			float h = 1.0f / (3.0f * 256) * v; 
			return h;
		}
	};
}
