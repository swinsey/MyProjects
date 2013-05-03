################################################################################
## @file:	files.mk
## @author	郭春茂 <gcm.ustc.edu>
## @brief	源文件、中间文件和子目录
## @version	1.0
###############################################################################

VPATH   		:= $(DEPEND_DIRECTORY):$(OBJECT_DIRECTORY)

SOURCE_DEPTH    	:= $(PROJECT_SOURCE_DEPTH)

ifeq ($(SOURCE_DEPTH),)
	SOURCE_DEPTH	        := 1
endif

SOURCE_SUB_DIRECTORYS	:= $(PROJECT_SOURCE_SUB_DIRECTORYS)

ifeq ($(SOURCE_SUB_DIRECTORYS),)

	ifeq ($(SOURCE_DEPTH),1)
		SOURCE_SUB_DIRECTORYS	:= *
	endif

	ifeq ($(SOURCE_DEPTH),2)
		SOURCE_SUB_DIRECTORYS	:= * */*
	endif

	ifeq ($(SOURCE_DEPTH),3)
		SOURCE_SUB_DIRECTORYS	:= * */* */*/*
	endif
endif

SOURCES			:= $(PROJECT_SOURCES)

ifeq ($(SOURCES),)

	SOURCES			:= $(wildcard $(SOURCE_DIRECTORY)/*$(SOURCE_SUFFIX)) $(foreach dir,$(SOURCE_SUB_DIRECTORYS),$(wildcard $(SOURCE_DIRECTORY)/$(dir)/*$(SOURCE_SUFFIX)))

	SOURCES			:= $(patsubst $(SOURCE_DIRECTORY)/%,%,$(SOURCES))
endif

SOURCE_SUB_DIRECTORYS	:= $(sort $(filter-out .,$(patsubst %/,%,$(dir $(SOURCES)))))

SOURCE_DEPENDS		:= $(patsubst %$(SOURCE_SUFFIX),%.dep,$(SOURCES))

SOURCE_OBJECTS		:= $(patsubst %$(SOURCE_SUFFIX),%.o,$(SOURCES))

HEADERS			:= $(PROJECT_COMMON_HEADERS)

HEADER_DEPENDS		:= $(addsuffix .dep,$(HEADERS))

HEADER_SUB_DIRECTORYS	:= $(sort $(filter-out .,$(patsubst %/,%,$(dir $(HEADERS)))))

HEADER_OBJECTS		:= $(addsuffix .gch,$(HEADERS))

OBJECT_SUB_DIRECTORYS	:= $(sort $(SOURCE_SUB_DIRECTORYS) $(HEADER_SUB_DIRECTORYS))

DEPEND_SUB_DIRECTORYS	:= $(addprefix $(DEPEND_DIRECTORY)/, $(OBJECT_SUB_DIRECTORYS))

OBJECT_SUB_DIRECTORYS	:= $(addprefix $(OBJECT_DIRECTORY)/, $(OBJECT_SUB_DIRECTORYS))
