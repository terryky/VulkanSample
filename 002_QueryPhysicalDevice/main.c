
#include <stdio.h>
#include <assert.h>
#include <vulkan/vulkan.h>

#define APP_NAME "VulkanSample"
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

struct StructHeader {
	VkStructureType sType;
	void            *pNext;
};

struct StructChainInfo {
	VkStructureType sType;
	uint32_t        mem_size;
};


/* メッセンジャコールバック */
VKAPI_ATTR VkBool32 VKAPI_CALL
dbgMessengerCb(
	VkDebugUtilsMessageSeverityFlagBitsEXT     messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT            messageType,
	const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
	void                                       *pUserData)
{
	uint32_t i;

	fprintf(stderr, "===========================================\n");
	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) fprintf(stderr, "[VERBOSE]");
	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)    fprintf(stderr, "[INFO]");
	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) fprintf(stderr, "[WARNING]");
	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)   fprintf(stderr, "[ERROR]");

	if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)         fprintf(stderr, "(GENERAL)");
	if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)      fprintf(stderr, "(VALIDATION)");
	if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)     fprintf(stderr, "(PERFORMANCE)");

	fprintf(stderr, "\n");
	fprintf(stderr, "  MessageID: %d\n", pCallbackData->messageIdNumber);
	fprintf(stderr, "  MessageID: %s\n", pCallbackData->pMessageIdName);
	fprintf(stderr, "  Message  : %s\n", pCallbackData->pMessage);

	for (i = 0; i < pCallbackData->objectCount; i++)
	{
		fprintf(stderr, "  ---Object [%d/%d] ", i, pCallbackData->objectCount);
		fprintf(stderr, "objectType: %d, Handle:%p, Name:%s\n",
			pCallbackData->pObjects[i].objectType,
			(void*)pCallbackData->pObjects[i].objectHandle,
			pCallbackData->pObjects[i].pObjectName);
	}

	for (i = 0; i < pCallbackData->cmdBufLabelCount; i++)
	{
		fprintf(stderr, "  ---Command Buffer Labes [%d/%d] ", i, pCallbackData->cmdBufLabelCount);
		fprintf(stderr, "%s {%f, %f, %f, %f}\n",
			pCallbackData->pCmdBufLabels[i].pLabelName,
			pCallbackData->pCmdBufLabels[i].color[0],
			pCallbackData->pCmdBufLabels[i].color[1],
			pCallbackData->pCmdBufLabels[i].color[2],
			pCallbackData->pCmdBufLabels[i].color[3]);
	}


	/* この関数は常にFALSEを返すべき。TRUEは将来拡張用 */
	return VK_FALSE;
}


static void 
BuildStructChain(struct StructHeader *first, const struct StructChainInfo *chain_info,
				 uint32_t chain_info_len) 
{
	uint32_t i;
	struct StructHeader *place = first;

	for (i = 0; i < chain_info_len; i++) 
	{
		place->pNext = malloc(chain_info[i].mem_size);
		if (!place->pNext) 
		{
			fprintf(stderr, "FATAL ERROR: %s(%d).\n", __FILE__, __LINE__);
			exit(1);
		}
		place = place->pNext;
		place->sType = chain_info[i].sType;
	}

	place->pNext = NULL;
}


VkInstance
init_vulkan_instance(void)
{
	VkInstance instance;
	VkResult err;
	uint32_t enabled_extension_count = 0;
	uint32_t enabled_layer_count = 0;

	
	const VkApplicationInfo app_info = {
		.sType                 = VK_STRUCTURE_TYPE_APPLICATION_INFO,	// この構造体の型。必ずVK_STRUCTURE_TYPE_APPLICATION_INFOを指定
		.pNext                 = NULL,									// 拡張機能用のポインタ
		.pApplicationName      = APP_NAME,								// アプリ名
		.applicationVersion    = 0,										// アプリバージョン
		.pEngineName           = APP_NAME,								// アプリを作るためのエンジン（もしあれば）
		.engineVersion         = 0,										// エンジンのバージョン
		.apiVersion            = VK_API_VERSION_1_0,					// アプリが使いたいVulkanのバージョン
	};


	/*
	* CreateInstance中に使う「一時的な」コールバック関数のための情報
	* インスタンス生成後は、コールバック関数の登録に、インスタンスベースな関数を使う。
	*/
	VkDebugUtilsMessengerCreateInfoEXT dbg_msg_info = {
		.sType                 = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.pNext                 = NULL,
		.flags                 = 0,
		.messageSeverity       = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |	// バグ診断用の冗長メッセージ
		                         //VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT		// リソース情報等|
		                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |	// アプリの不具合
						         VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,		// アプリクラッシュなどを引き起こす深刻な不具合
		.messageType           = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |		// 一般的なイベント	
							     VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |	// 不正な振る舞いを起こす
								 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,	// Vulkanの使い方が最適かされてない
		.pfnUserCallback       = dbgMessengerCb,
		.pUserData             = NULL,
	};


	VkInstanceCreateInfo inst_info = {
		.sType                 = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,	// この構造体の型。必ずVK_STRUCTURE_TYPE_INSTANCE_CREATE_INFOを指定
		.pNext                 = &dbg_msg_info,								// 拡張機能用のポインタ
		.flags                 = 0,											// reserve
		.pApplicationInfo      = &app_info,
		.enabledLayerCount     = enabled_layer_count,						// 有効化するグローバルレイヤの数
		.ppEnabledLayerNames   = NULL,										// グローバルレイヤの名前
		.enabledExtensionCount = enabled_extension_count,					// 有効化するグローバル拡張の数
		.ppEnabledExtensionNames = NULL,									// グローバル拡張の名前
	};


	/*
	 *	Vulkan インスタンスを作る。
	 *
	 *		・リクエストしたレイヤが存在するか確認
	 *		・リクエストした拡張機能が存在するか確認
	 *		・VKインスタンスを生成し、アプリに返す
	 */
	err = vkCreateInstance (&inst_info,	// [in ] インスタンス構築情報
							NULL,		// [in ] 必要ならホストメモリアロケータを指定
							&instance);	// [out] 作成されたインスタンスが格納される
	if (err == VK_SUCCESS)
	{
		uint32_t version;
		vkEnumerateInstanceVersion(&version);

		fprintf(stderr, "vkCreateInstance success.\n");
		fprintf(stderr, "VKInstance Version = %d.%d.%d\n",
			VK_VERSION_MAJOR(version), VK_VERSION_MINOR(version), VK_VERSION_PATCH(version));
	}
	/* 互換性のあるVulkanドライバが見つからなかった */
	else if (err == VK_ERROR_INCOMPATIBLE_DRIVER) 
	{
		fprintf(stderr, "vkCreateInstance Failure: [VK_ERROR_INCOMPATIBLE_DRIVER].\n");
    }
	/* 指定した拡張機能が見つからなかった */
	else if (err == VK_ERROR_EXTENSION_NOT_PRESENT) 
	{
		fprintf(stderr, "vkCreateInstance Failure: [VK_ERROR_EXTENSION_NOT_PRESENT].\n");
    } 
	else if (err) 
	{
		fprintf(stderr, "vkCreateInstance Failure: [%x].\n", err);
    }

	return instance;
}

/*
 * 物理デバイス：物理的に接続されたハードウェア（GPU）そのもの
 * 論理デバイス：アプリから見た時のデバイス
 */
VkPhysicalDevice
query_vulkan_device(VkInstance vk_instance)
{
	VkResult err;
	uint32_t gpu_count, i, j;
	VkPhysicalDevice phydev, *phydev_array;

	/* 
	 *  Vulkan物理デバイスの個数を取得 
	 *	（第3引数にNULL指定）
	 */
	err = vkEnumeratePhysicalDevices(vk_instance, /* [in ] VKインスタンス */ 
									 &gpu_count,  /* [out] 物理デバイス数が格納される */
									 NULL);       /* [out] VkPhysicalDeviceハンドルが格納される */
	assert(!err);

	if (gpu_count == 0)
	{
		fprintf(stderr, "vkEnumeratePhysicalDevices() returned no devices.\n");
		exit(1);
	}

	/* VkPhysicalDeviceハンドルを一気に取得するための領域確保 */
	phydev_array = malloc(sizeof(VkPhysicalDevice) * gpu_count);

	/*
	 *  改めて、VkPhysicalDeviceハンドルを一気に取得
	 *	（第3引数に格納先となる配列へのポインタ指定）
	 */
	err = vkEnumeratePhysicalDevices(vk_instance, &gpu_count, phydev_array);
	assert(!err);

	/* 
	 * VkPhysicalDeviceハンドル からさらに詳細プロパティを取得 
	 */
	for (i = 0; i < gpu_count; i ++)
	{
		phydev = phydev_array[i];
		VkPhysicalDeviceProperties  dev_props;

		/* ----------------------------------------------------- *
		 * VkPhysicalDevice ごとに、デバイス情報を調べる
		 * ----------------------------------------------------- */
		vkGetPhysicalDeviceProperties(phydev,		/* [in ] Vulkan物理デバイスのハンドル */
									  &dev_props);	/* [out] Vulkan物理デバイス情報が格納される */

		fprintf(stderr, "================ VulkanPhysicalDevice[%d/%d] ================\n", i, gpu_count);
		fprintf(stderr, "%s\n", dev_props.deviceName);
		fprintf(stderr, "apiVersion = %d.%d.%d\n",
			VK_VERSION_MAJOR(dev_props.apiVersion),
			VK_VERSION_MINOR(dev_props.apiVersion),
			VK_VERSION_PATCH(dev_props.apiVersion));


		/* ----------------------------------------------------- *
		 * VkPhysicalDevice ごとに、使える Extension を調べる
		 * ----------------------------------------------------- */
		uint32_t ext_count = 0;
		VkExtensionProperties *ext_prop = NULL;

		err = vkEnumerateDeviceExtensionProperties(phydev, NULL, &ext_count, NULL);
		assert(!err);

		ext_prop = malloc(ext_count * sizeof(VkExtensionProperties));
		err = vkEnumerateDeviceExtensionProperties(phydev, NULL, &ext_count, ext_prop);
		assert(!err);

		for (j = 0; j < ext_count; j++)
		{
			fprintf(stderr, "[%2d/%2d] %s (%d)\n", 
				j, ext_count, ext_prop[j].extensionName, ext_prop[j].specVersion);
		}


		/* ----------------------------------------------------- *
		 * vkGetPhysicalDeviceProperties2() は、
		 * VkPhysicalDevice の API バージョンが 1.1 以上じゃないと使えない 
		 * (VkInstance のバージョンが 1.1 以上でもダメ。)
		 * ----------------------------------------------------- */
		if (dev_props.apiVersion >= VK_MAKE_VERSION(1, 1, 0))
		{
			VkPhysicalDeviceProperties2 dev_props2;

			/*
			 *	vkGetPhysicalDeviceProperties2() を使う前には、
			 *	事前にプロパティ構造体のチェーンを準備しておく必要がある
			 */
			struct StructChainInfo chain_info[] = {
				{ .sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_PROPERTIES_EXT,
				  .mem_size = sizeof(VkPhysicalDeviceBlendOperationAdvancedPropertiesEXT) },
				{ .sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_POINT_CLIPPING_PROPERTIES_KHR,
				  .mem_size = sizeof(VkPhysicalDevicePointClippingPropertiesKHR) },
				{ .sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR,
				  .mem_size = sizeof(VkPhysicalDevicePushDescriptorPropertiesKHR) },
				{ .sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DISCARD_RECTANGLE_PROPERTIES_EXT,
				  .mem_size = sizeof(VkPhysicalDeviceDiscardRectanglePropertiesEXT) },
				{ .sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES_KHR,
				  .mem_size = sizeof(VkPhysicalDeviceMultiviewPropertiesKHR) } };

			uint32_t chain_info_len = ARRAY_SIZE(chain_info);

			dev_props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
			BuildStructChain((struct StructHeader *)&dev_props2, chain_info, chain_info_len);

			vkGetPhysicalDeviceProperties2(phydev,			/* [in ] Vulkan物理デバイスのハンドル */
										   &dev_props2);	/* [out] Vulkan物理デバイス情報が格納される */

			VkPhysicalDeviceBlendOperationAdvancedPropertiesEXT *pBlendOp = dev_props2.pNext;
		}


		/* ----------------------------------------------------- *
		 *	VkPhysicalDevice ごとに、使えるQueueファミリ情報取得
		 * ----------------------------------------------------- */
		uint32_t qfamily_count;
		VkQueueFamilyProperties *queue_props;

		/* まず数を取得 */
		vkGetPhysicalDeviceQueueFamilyProperties(phydev, &qfamily_count, NULL);
		assert(qfamily_count >= 1);

		/* 領域確保して一気に取得 */
		queue_props = (VkQueueFamilyProperties *)malloc(qfamily_count * sizeof(VkQueueFamilyProperties));
		vkGetPhysicalDeviceQueueFamilyProperties(phydev, &qfamily_count, queue_props);

		for (j = 0; j < qfamily_count; j++)
		{
			fprintf(stderr, "QueueFamily[%d/%d] queueFlags:", j, qfamily_count);

			/* グラフィクス描画するにはこのビットが立ってるQueueFamilyを使う */
			if (queue_props[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)       fprintf(stderr, "GRAPHICS ");

			/* コンピューティング実行にはこのビットが立ってるQueueFamilyを使う */
			if (queue_props[j].queueFlags & VK_QUEUE_COMPUTE_BIT )       fprintf(stderr, "COMPUTE ");

			/* メモリ転送、テクスチャ転送にはこのビットが立ってるQueueFamilyを使う */
			if (queue_props[j].queueFlags & VK_QUEUE_TRANSFER_BIT)       fprintf(stderr, "TRANSFER ");

			/* スパースメモリ管理操作 */
			if (queue_props[j].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) fprintf(stderr, "SPARSE ");

			/* VK_DEVICE_QUEUE_CREATE_PROTECTED_BIT をサポート */
			if (queue_props[j].queueFlags & VK_QUEUE_PROTECTED_BIT)      fprintf(stderr, "PROTECTED ");
			fprintf(stderr, "\n");
		}

		/* ----------------------------------------------------- *
		 *	VkPhysicalDevice ごとに、使える機能の情報取得
		 * ----------------------------------------------------- */
		VkPhysicalDeviceFeatures physDevFeatures;
		vkGetPhysicalDeviceFeatures(phydev, &physDevFeatures);


		/*
		 *	vkGetPhysicalDeviceFormatProperties() で、使用可能なカラーフォーマット情報取得
		 *	vkGetPhysicalDeviceMemoryProperties() で、使用可能なメモリ情報取得
		 */
	}


	/* とりあえず最初の物理デバイスを選ぶ */
	phydev = phydev_array[0];

	free(phydev_array);

	return phydev;
}




int
main(int argc, char *argv[])
{
	VkInstance vk_instance;
	VkPhysicalDevice vk_phydev;

	vk_instance = init_vulkan_instance();

	vk_phydev = query_vulkan_device(vk_instance);

	return 0;
}

