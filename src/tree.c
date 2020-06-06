#include "flecs_private.h"
#include "flecs/util/strbuf.h"

static
bool path_append(
    ecs_world_t *world, 
    ecs_entity_t parent, 
    ecs_entity_t child, 
    ecs_entity_t component,
    char *sep,
    char *prefix,
    ecs_strbuf_t *buf)
{
    ecs_type_t type = ecs_get_type(world, child);
    ecs_entity_t cur = ecs_find_in_type(world, type, component, ECS_CHILDOF);
    
    if (cur) {
        if (cur != parent) {
            path_append(world, parent, cur, component, sep, prefix, buf);
            ecs_strbuf_appendstr(buf, sep);
        }
    } else if (prefix) {
        ecs_strbuf_appendstr(buf, prefix);
    }

    ecs_strbuf_appendstr(buf, ecs_get_name(world, child));

    return cur != 0;
}

char* ecs_get_path_w_sep(
    ecs_world_t *world,
    ecs_entity_t parent,
    ecs_entity_t child,
    ecs_entity_t component,
    char *sep,
    char *prefix)
{
    ecs_strbuf_t buf = ECS_STRBUF_INIT;

    if (parent != child) {
        path_append(world, parent, child, component, sep, prefix, &buf);
    } else {
        ecs_strbuf_appendstr(&buf, "");
    }

    return ecs_strbuf_get(&buf);
}

static
ecs_entity_t find_child_in_table(
    ecs_world_t *world,
    ecs_stage_t *stage,
    ecs_table_t *table,
    int32_t name_index,
    const char *name)
{
    ecs_data_t *data = ecs_table_get_staged_data(world, stage, table);
    if (!data || !data->columns) {
        return 0;
    }

    int32_t i, count = ecs_vector_count(data->entities);
    if (!count) {
        return 0;
    }

    ecs_column_t *column = &data->columns[name_index];
    EcsName *names = ecs_vector_first(column->data, EcsName);

    for (i = 0; i < count; i ++) {
        if (!strcmp(names[i], name)) {
            return *ecs_vector_get(data->entities, ecs_entity_t, i);
        }
    }

    return 0;
}

static
ecs_entity_t find_child_in_staged(
    ecs_world_t *world,
    ecs_stage_t *stage,
    ecs_entity_t parent,
    const char *name)
{
    ecs_sparse_each(stage->tables, ecs_table_t, table, {
        ecs_type_t type = table->type;

        /* If table doesn't have EcsName, then don't bother */
        int32_t name_index = ecs_type_index_of(type, ecs_entity(EcsName));
        if (name_index == -1) {
            continue;
        }

        ecs_entity_t result = find_child_in_table(world, stage, table, 
            name_index, name);
        if (result) {
            return result;
        }
    });    

    return 0;
}

ecs_entity_t ecs_lookup_child(
    ecs_world_t *world,
    ecs_entity_t parent,
    const char *name)
{
    ecs_entity_t result = 0;
    ecs_stage_t *stage = ecs_get_stage(&world);

    ecs_vector_t *child_tables = ecs_map_get_ptr(
        world->child_tables, ecs_vector_t*, parent);
    
    if (child_tables) {
        ecs_vector_each(child_tables, ecs_table_t*, table_ptr, {
            ecs_table_t *table = *table_ptr;
            ecs_type_t type = table->type;

            /* If table doesn't have EcsName, then don't bother */
            int32_t name_index = ecs_type_index_of(type, ecs_entity(EcsName));
            if (name_index == -1) {
                continue;
            }

            result = find_child_in_table(world, stage, table, name_index, name);
            if (!result) {
                if (stage != &world->stage) {
                    result = find_child_in_table(world, &world->stage, table, 
                        name_index, name);
                }
            }

            if (result) {
                return result;
            }
        });
    }

    /* If child hasn't been found it is possible that it was
     * created in a new table while staged, and the table hasn't
     * been registered with the child_table map yet. In that case we
     * have to look for the entity in the staged tables.
     * This edge case should rarely result in a lot of overhead,
     * since the number of tables should stabilize over time, which
     * means table creation while staged should be infrequent. */    
    if (!result && stage != &world->stage) {
        result = find_child_in_staged(world, stage, parent, name);
    }

    return result;
}

ecs_entity_t ecs_lookup(
    ecs_world_t *world,
    const char *name)
{   
    if (!name) {
        return 0;
    }

    if (isdigit(name[0])) {
        return atoi(name);
    }
    
    return ecs_lookup_child(world, 0, name);
}

static
bool is_sep(
    const char **ptr,
    const char *sep)
{
    size_t len = strlen(sep);

    if (!strncmp(*ptr, sep, len)) {
        *ptr += len - 1;
        return true;
    } else {
        return false;
    }
}

ecs_entity_t ecs_lookup_path_w_sep(
    ecs_world_t *world,
    ecs_entity_t parent,
    const char *path,
    const char *sep,
    const char *prefix)
{
    char buff[ECS_MAX_NAME_LENGTH];
    const char *ptr;
    char ch, *bptr;
    ecs_entity_t cur = parent;

    if (prefix) {
        size_t len = strlen(prefix);
        if (!strncmp(path, prefix, len)) {
            path += len;
            cur = 0;
        }
    }

    for (bptr = buff, ptr = path; (ch = *ptr); ptr ++) {
        if (is_sep(&ptr, sep)) {
            *bptr = '\0';
            bptr = buff;
            cur = ecs_lookup_child(world, cur, buff);
            if (!cur) {
                return 0;
            }
        } else {
            *bptr = ch;
            bptr ++;
        }
    }

    if (bptr != buff) {
        *bptr = '\0';
        cur = ecs_lookup_child(world, cur, buff);
    }

    return cur;
}

ecs_view_t ecs_tree_iter(
    ecs_world_t *world,
    ecs_entity_t parent)
{
    ecs_tree_iter_t iter = {
        .tables = ecs_map_get_ptr(world->child_tables, ecs_vector_t*, parent),
        .index = 0
    };

    return (ecs_view_t) {
        .world = world,
        .iter.parent = iter
    };
}

bool ecs_tree_next(
    ecs_view_t *view)
{
    ecs_tree_iter_t *iter = &view->iter.parent;
    ecs_vector_t *tables = iter->tables;
    int32_t count = ecs_vector_count(tables);
    int32_t i;

    for (i = iter->index; i < count; i ++) {
        ecs_table_t *table = *ecs_vector_get(tables, ecs_table_t*, i);
        ecs_assert(table != NULL, ECS_INTERNAL_ERROR, NULL);
        
        ecs_data_t *data = ecs_vector_first(table->stage_data, ecs_data_t);
        if (!data) {
            continue;
        }

        view->count = ecs_table_count(table);
        if (!view->count) {
            continue;
        }

        view->table = table;
        view->table_columns = data->columns;
        view->count = ecs_table_count(table);
        view->entities = ecs_vector_first(data->entities, ecs_entity_t);
        iter->index = ++i;

        return true;
    }

    return false;    
}
