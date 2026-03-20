#include <engine.h>

int main(int argc, char* argv[])
{
    Engine engine = {0};
	
    engine_init(&engine);

    engine_loop(&engine);

    engine_cleanup(&engine);
}