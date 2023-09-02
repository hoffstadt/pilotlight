#include "pl_json_tests.h"
#include "pl_math_tests.h"
#include "pl_ds_tests.h"

int main()
{
    plTestContext* ptTestContext = pl_create_test_context();
    
    // data structure tests
    pl_test_register_test(hashmap_test_0, NULL);

    // json tests
    pl_test_register_test(json_test_0, NULL);
    
    // math tests
    pl_test_register_test(math_test_0, NULL);

    if(!pl_test_run())
    {
        exit(1);
    }

    return 0;
}

#define PL_JSON_IMPLEMENTATION
#include "pl_json.h"

#define PL_MATH_IMPLEMENTATION
#include "pl_math.h"

#define PL_TEST_IMPLEMENTATION
#include "pl_test.h"
