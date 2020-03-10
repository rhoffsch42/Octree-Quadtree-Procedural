# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: rhoffsch <rhoffsch@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2015/05/15 17:38:19 by rhoffsch          #+#    #+#              #
#    Updated: 2020/03/10 05:02:52 by rhoffsch         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME			=	Trees
CC				=	g++ -std=c++17
CFLAGS			=	-Wall -Wextra -Werror -MMD -g -fsanitize=address #-Wpadded

INCLUDE			=	-I ${GLEW_DIR}/include \
					-I ${GLFW_DIR}/include \
					-I $(SGL_DIR)/include \
					-I include

GLEW			=	${GLEW_DIR}/lib/libGLEW.dylib
GLFW			=	${GLFW_DIR}/lib/libglfw.dylib

FRAMEWORKS		=	-framework OpenGL #-framework Cocoa
CC_NEEDS		=	$(FRAMEWORKS) $(GLFW) $(GLEW)

SRC_FILE		=	main.cpp \
					quadtree.cpp

HDR_FILE		=	trees.h

SRC_DIR			=	src
OBJ_DIR			=	obj
HDR_DIR			=	include
SRC				=	$(addprefix $(SRC_DIR)/, $(SRC_FILE))
OBJ				=	$(patsubst %.cpp, $(OBJ_DIR)/%.o, $(SRC_FILE))
DPD				=	$(patsubst %.cpp, $(OBJ_DIR)/%.d, $(SRC_FILE))
HDR				=	$(addprefix $(HDR_DIR)/, $(HDR_FILE))

##################################### SimpleGL
SGL_SRC_FILE	=	blueprint.cpp \
					cam.cpp \
					glfw.cpp \
					math.cpp \
					misc.cpp \
					properties.cpp \
					object.cpp \
					obj3d.cpp \
					obj3dBP.cpp \
					obj3dPG.cpp \
					program.cpp \
					skybox.cpp \
					skyboxPG.cpp \
					texture.cpp \
					behavior.cpp \
					behaviormanaged.cpp \
					transformBH.cpp \
					fps.cpp \
					gamemanager.cpp \
					framebuffer.cpp \
					uipanel.cpp

SGL_DIR			=	SimpleGL
SGL_OBJ_DIR		=	$(SGL_DIR)/obj
SGL_OBJ			=	$(patsubst %.cpp, $(SGL_OBJ_DIR)/%.o, $(SGL_SRC_FILE))
##################################### SimpleGL end

ALL_OBJ			=	$(OBJ)
ALL_OBJ			+=	$(SGL_OBJ)

DEPENDS			=	$(OBJ:.o=.d)

.PHONY: all clean fclean pclean re tests submodule run

release:
	@make -j -C $(SGL_DIR) && \
	make -j all

all: $(NAME)
	@echo $(NAME) > .gitignore

$(NAME): $(OBJ)
	@ $(CC) $(CFLAGS) $(ALL_OBJ) -o $(NAME) $(LIBS) $(FRAMEWORKS) $(GLFW) $(GLEW)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp Makefile | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

pclean: fclean
	@$(MAKE) pclean -C $(SGL_DIR)

re:
	@make fclean
	@make release

submodule:
	git submodule update --init --recursive

run: all
	@echo ""
	./$(NAME)
	@echo ""
	@echo "exitcode: $$?"

-include $(DPD)
