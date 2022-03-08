#include "Pool.h"
#include <iostream>

int main()
{
	DAOS_CHECK(daos_init());

	uuid_t pool_uuid; /** pool UUID */
	DAOS_CHECK(uuid_parse("27511db3-9fc3-4714-b1b3-1827c6a7b96b", pool_uuid));

	Pool pool(pool_uuid);

	auto container = pool.add_container("name");

	std::cout << "Created container UUID: " << unparse(container->get_uuid()) << std::endl;

	pool.remove_container(container);

	DAOS_CHECK(daos_fini());
}