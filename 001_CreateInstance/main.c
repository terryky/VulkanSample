
#include <stdio.h>
#include <assert.h>
#include <vulkan/vulkan.h>

#define APP_NAME "VulkanSample"



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
		fprintf(stderr, "vkCreateInstance success.\n");
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



int
main(int argc, char *argv[])
{
	VkInstance vk_instance;

	vk_instance = init_vulkan_instance();

	return 0;
}

