# 直播多进程策略

ifeq ($(LOCAL_NAME),/ppbox/live_worker)
	PROJECT_COMPILE_FLAGS	:= $(PROJECT_COMPILE_FLAGS) -DPPBOX_LIVE_WORKER_MULTI_PROCESS
endif
