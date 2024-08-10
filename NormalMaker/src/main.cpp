#include "vkpch.h"
#include "core/VulkanApplication.h"

#include "layer/VulkanLayer.h"

int main(int argc, char** argv)
{
	VulkanApplication app;
	app.PushLayer<VulkanLayer>();

	if (!app.Init())
		return -1;

	app.Run();
	app.Destroy();

	return 0;
}
