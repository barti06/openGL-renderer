#include <engine.h>
#include <stdalign.h>

int main(int argc, char* argv[])
{
    Engine engine = {0};
	
    engine_init(&engine, argc, argv);

    engine_loop(&engine);

    engine_cleanup(&engine);
}
