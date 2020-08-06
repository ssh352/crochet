#include "hashmap.h"

static void
_map_list_add(struct _map_list **list, void *ori_key, void *value)
{
	// edge case for first value since the first value is NULL
	if (!(*list)) {
		*list = malloc(sizeof(struct _map_list) * 1);
		(*list)->key = ori_key;
		(*list)->value = value;
		(*list)->next = NULL;
		return;
	}

	// loop until the last element
	struct _map_list *iter = *list;
	while (iter->next != NULL) {
		iter = iter->next;
	}

	// iterator is now the last element
	iter->next = malloc(sizeof(struct _map_list) * 1);
	iter = iter->next;
	iter->key = ori_key;
	iter->value = value;
	iter->next = NULL;
}

struct hashmap *
hashmap_new(unsigned int num_bins)
{
	struct hashmap *map = malloc(sizeof(struct hashmap) * 1);
	if (!map) {
		pprint_error(
		    "not enough memory", __FILE_NAME__, __func__, __LINE__);
		abort();
	}
	map->bins = malloc(sizeof(struct _map_list) * num_bins);
	if (!map->bins) {
		pprint_error(
		    "not enough memory", __FILE_NAME__, __func__, __LINE__);
		abort();
	}
	for (unsigned int i = 0; i < num_bins; ++i) {
		map->bins[i] = NULL;
	}
	map->num_bins = num_bins;
	return map;
}

void
hashmap_put(void *key, void *value, struct hashmap *map)
{
	if (!key || !value || !map) {
		pprint_error("key, value, or map points to null", __FILE_NAME__,
		    __func__, __LINE__);
		abort();
	}

	size_t hash = (size_t)key;
	size_t bin = hash % map->num_bins;

	struct _map_list **ll = &(map->bins[bin]);
	_map_list_add(ll, key, value);
}

void *
hashmap_get(void *key, struct hashmap *map)
{
	size_t hash = (size_t)key;
	size_t bin = hash % map->num_bins;

	struct _map_list *iter = map->bins[bin];

	while (iter->next != NULL) {
		if (iter->key == key) {
			return iter->value;
		}
		iter = iter->next;
	}

	// key doesn't exist
	return NULL;
}
