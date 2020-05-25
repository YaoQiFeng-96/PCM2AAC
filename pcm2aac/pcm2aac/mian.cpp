#include "Pcm2AAC.h"

#pragma warning(disable:4996)

int main(int argc, char* argv[])
{
	CPcm2AAC	pcm2aac;
	if (!pcm2aac.Init(8000, AV_SAMPLE_FMT_S16, 1))
	{
		std::cout << "pcm2aac init failed." << std::endl;
		return -1;
	}

	FILE *in_file = fopen("8k_1_16.pcm", "rb");
	FILE *out_file = fopen("8k_1_16.aac", "wb");

	uint8_t read_buf[1024] = { 0 };
	uint8_t* pData = nullptr;
	int size = 0;

	while (!feof(in_file))
	{
		int iRead = fread(read_buf, sizeof(uint8_t), 1024, in_file);
		if (iRead <= 0)
		{
			break;
		}
		pcm2aac.AddData(read_buf, iRead);

		while (pcm2aac.GetData(pData, &size))
		{
			fwrite(pData, 1, size, out_file);
			free(pData);
			pData = nullptr;
			size = 0;
		}
	}

	fclose(in_file);
	fclose(out_file);
	return 0;
}