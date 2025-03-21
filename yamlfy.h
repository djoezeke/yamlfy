#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define YAML value types
typedef enum
{
    SCALAR,
    SEQUENCE,
    MAPPING,
    BOOLEAN,
    NIL,
    INTEGER,
    DOUBLE,
    STRING,
} YamlType;

inline const char *to_string(YamlType t)
{
    switch (t)
    {
    case SEQUENCE:
        return "SEQUENCE";
    case MAPPING:
        return "MAPPING";
    case NIL:
        return "NULL";
    case BOOLEAN:
        return "BOOLEAN";
    case INTEGER:
        return "INTEGER";
    case DOUBLE:
        return "DOUBLE";
    case STRING:
        return "STRING";
    default: // LCOV_EXCL_LINE
    }
}

// Forward declaration of Yaml
typedef struct Yaml Yaml;

bool yaml_to_bool(Yaml *yaml);
double yaml_to_double(Yaml *yaml);
int yaml_to_int(Yaml *yaml);
char *yaml_to_string(Yaml *yaml);
void *yaml_to_null(Yaml *yaml);

// Define a YAML key-value pair for mappings
typedef struct
{
    char *key;
    Yaml *value;
} YamlKeyValuePair;

// Define a YAML value
struct Yaml
{
    YamlType type;
    union
    {
        char *scalar_value;
        struct
        {
            Yaml **items;
            size_t size;
        } sequence_value;
        struct
        {
            YamlKeyValuePair **items;
            size_t size;
        } mapping_value;
    } data;
};

// Function to create a YAML scalar value
Yaml *yaml_create_scalar(const char *scalar_value)
{
    Yaml *value = (Yaml *)malloc(sizeof(Yaml));
    value->type = SCALAR;
    value->data.scalar_value = strdup(scalar_value);
    return value;
}

// Function to create a YAML sequence value
Yaml *yaml_create_sequence()
{
    Yaml *value = (Yaml *)malloc(sizeof(Yaml));
    value->type = SEQUENCE;
    value->data.sequence_value.items = NULL;
    value->data.sequence_value.size = 0;
    return value;
}

// Function to add an item to a YAML sequence
void yaml_sequence_add(Yaml *sequence, Yaml *item)
{
    if (sequence->type != SEQUENCE)
        return;
    sequence->data.sequence_value.items = (Yaml **)realloc(sequence->data.sequence_value.items, (sequence->data.sequence_value.size + 1) * sizeof(Yaml *));
    sequence->data.sequence_value.items[sequence->data.sequence_value.size] = item;
    sequence->data.sequence_value.size++;
}

// Function to create a YAML mapping value
Yaml *yaml_create_mapping()
{
    Yaml *value = (Yaml *)malloc(sizeof(Yaml));
    value->type = MAPPING;
    value->data.mapping_value.items = NULL;
    value->data.mapping_value.size = 0;
    return value;
}

// Function to add a key-value pair to a YAML mapping
void yaml_mapping_add(Yaml *mapping, const char *key, Yaml *value)
{
    if (mapping->type != MAPPING)
        return;
    YamlKeyValuePair *pair = (YamlKeyValuePair *)malloc(sizeof(YamlKeyValuePair));
    pair->key = strdup(key);
    pair->value = value;
    mapping->data.mapping_value.items = (YamlKeyValuePair **)realloc(mapping->data.mapping_value.items, (mapping->data.mapping_value.size + 1) * sizeof(YamlKeyValuePair *));
    mapping->data.mapping_value.items[mapping->data.mapping_value.size] = pair;
    mapping->data.mapping_value.size++;
}

// Function to print a YAML value (for demonstration purposes)
void yaml_print(Yaml *value, int indent)
{
    for (int i = 0; i < indent; i++)
        printf("  ");
    switch (value->type)
    {
    case SCALAR:
        printf("%s\n", value->data.scalar_value);
        break;
    case SEQUENCE:
        printf("-\n");
        for (size_t i = 0; i < value->data.sequence_value.size; i++)
        {
            yaml_print(value->data.sequence_value.items[i], indent + 1);
        }
        break;
    case MAPPING:
        for (size_t i = 0; i < value->data.mapping_value.size; i++)
        {
            for (int j = 0; j < indent; j++)
                printf("  ");
            printf("%s:\n", value->data.mapping_value.items[i]->key);
            yaml_print(value->data.mapping_value.items[i]->value, indent + 1);
        }
        break;
    }
}

// Function to pretty print a YAML value
void yaml_pretty_print(Yaml *value, int indent)
{
    for (int i = 0; i < indent; i++)
        printf("  ");
    switch (value->type)
    {
    case SCALAR:
        printf("%s\n", value->data.scalar_value);
        break;
    case SEQUENCE:
        for (size_t i = 0; i < value->data.sequence_value.size; i++)
        {
            for (int j = 0; j < indent; j++)
                printf("  ");
            printf("- ");
            yaml_pretty_print(value->data.sequence_value.items[i], indent + 1);
        }
        break;
    case MAPPING:
        for (size_t i = 0; i < value->data.mapping_value.size; i++)
        {
            for (int j = 0; j < indent; j++)
                printf("  ");
            printf("%s: ", value->data.mapping_value.items[i]->key);
            yaml_pretty_print(value->data.mapping_value.items[i]->value, indent + 1);
        }
        break;
    }
}

// Function to free a YAML value
void yaml_free(Yaml *value)
{
    if (!value)
        return;
    switch (value->type)
    {
    case SCALAR:
        free(value->data.scalar_value);
        break;
    case SEQUENCE:
        for (size_t i = 0; i < value->data.sequence_value.size; i++)
        {
            yaml_free(value->data.sequence_value.items[i]);
        }
        free(value->data.sequence_value.items);
        break;
    case MAPPING:
        for (size_t i = 0; i < value->data.mapping_value.size; i++)
        {
            free(value->data.mapping_value.items[i]->key);
            yaml_free(value->data.mapping_value.items[i]->value);
            free(value->data.mapping_value.items[i]);
        }
        free(value->data.mapping_value.items);
        break;
    default:
        break;
    }
    free(value);
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function to read the contents of a file into a string
char *read_file(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Failed to open file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = (char *)malloc(length + 1);
    if (!content)
    {
        perror("Failed to allocate memory");
        fclose(file);
        return NULL;
    }

    fread(content, 1, length, file);
    content[length] = '\0';

    fclose(file);
    return content;
}
#include <ctype.h>

// Function to skip whitespace characters
const char *skip_whitespace(const char *str)
{
    while (isspace(*str))
        str++;
    return str;
}

// Function to parse a scalar value
Yaml *parse_scalar(const char **str)
{
    const char *start = *str;
    while (**str && !isspace(**str) && **str != ':' && **str != '-')
        (*str)++;
    size_t length = *str - start;
    char *scalar_value = (char *)malloc(length + 1);
    strncpy(scalar_value, start, length);
    scalar_value[length] = '\0';
    return yaml_create_scalar(scalar_value);
}

// Function to parse a YAML value
Yaml *parse_yaml(const char **str);

// Function to parse a sequence
Yaml *parse_sequence(const char **str)
{
    Yaml *sequence = yaml_create_sequence();
    while (**str)
    {
        *str = skip_whitespace(*str);
        if (**str != '-')
            break;
        (*str)++;
        *str = skip_whitespace(*str);
        Yaml *item = parse_yaml(str);
        yaml_sequence_add(sequence, item);
    }
    return sequence;
}

// Function to parse a mapping
Yaml *parse_mapping(const char **str)
{
    Yaml *mapping = yaml_create_mapping();
    while (**str)
    {
        *str = skip_whitespace(*str);
        if (**str == '\0' || **str == '-')
            break;
        const char *key_start = *str;
        while (**str && **str != ':')
            (*str)++;
        size_t key_length = *str - key_start;
        char *key = (char *)malloc(key_length + 1);
        strncpy(key, key_start, key_length);
        key[key_length] = '\0';
        (*str)++;
        *str = skip_whitespace(*str);
        Yaml *value = parse_yaml(str);
        yaml_mapping_add(mapping, key, value);
    }
    return mapping;
}

// Function to parse a YAML value
Yaml *parse_yaml(const char **str)
{
    *str = skip_whitespace(*str);
    if (**str == '-')
        return parse_sequence(str);
    if (strchr(*str, ':'))
        return parse_mapping(str);
    return parse_scalar(str);
}

// Function to read a YAML file and convert it to a Yaml structure
Yaml *read_yaml_file(const char *filename)
{
    char *content = read_file(filename);
    if (!content)
        return NULL;
    const char *str = content;
    Yaml *root = parse_yaml(&str);
    free(content);
    return root;
}
// Helper function to append formatted text to a string buffer
void append_to_buffer(char **buffer, size_t *size, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    size_t needed = vsnprintf(NULL, 0, format, args) + 1;
    va_end(args);

    *buffer = (char *)realloc(*buffer, *size + needed);
    va_start(args, format);
    vsnprintf(*buffer + *size, needed, format, args);
    va_end(args);

    *size += needed - 1;
}

// Function to pretty print a YAML value to a string
void yaml_pretty_print_to_string(Yaml *value, int indent, char **buffer, size_t *size)
{
    for (int i = 0; i < indent; i++)
        append_to_buffer(buffer, size, "  ");
    switch (value->type)
    {
    case SCALAR:
        append_to_buffer(buffer, size, "%s\n", value->data.scalar_value);
        break;
    case SEQUENCE:
        for (size_t i = 0; i < value->data.sequence_value.size; i++)
        {
            for (int j = 0; j < indent; j++)
                append_to_buffer(buffer, size, "  ");
            append_to_buffer(buffer, size, "- ");
            yaml_pretty_print_to_string(value->data.sequence_value.items[i], indent + 1, buffer, size);
        }
        break;
    case MAPPING:
        for (size_t i = 0; i < value->data.mapping_value.size; i++)
        {
            for (int j = 0; j < indent; j++)
                append_to_buffer(buffer, size, "  ");
            append_to_buffer(buffer, size, "%s: ", value->data.mapping_value.items[i]->key);
            yaml_pretty_print_to_string(value->data.mapping_value.items[i]->value, indent + 1, buffer, size);
        }
        break;
    }
}

int main()
{
    // Create a YAML mapping
    Yaml *root = yaml_create_mapping();
    yaml_mapping_add(root, "name", yaml_create_scalar("John Doe"));
    yaml_mapping_add(root, "age", yaml_create_scalar("30"));

    // Create a YAML sequence and add it to the mapping
    Yaml *hobbies = yaml_create_sequence();
    yaml_sequence_add(hobbies, yaml_create_scalar("reading"));
    yaml_sequence_add(hobbies, yaml_create_scalar("swimming"));
    yaml_sequence_add(hobbies, yaml_create_scalar("coding"));
    yaml_mapping_add(root, "hobbies", hobbies);

    // Print the YAML value
    yaml_print(root, 0);

    // Pretty print the YAML value
    yaml_pretty_print(root, 0);

    // Free the YAML value
    yaml_free(root);

    return 0;
}

#include <stdarg.h>

// Helper function to append formatted text to a string buffer
void append_to_buffer(char **buffer, size_t *size, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    size_t needed = vsnprintf(NULL, 0, format, args) + 1;
    va_end(args);

    *buffer = (char *)realloc(*buffer, *size + needed);
    va_start(args, format);
    vsnprintf(*buffer + *size, needed, format, args);
    va_end(args);

    *size += needed - 1;
}

// Function to pretty print a YAML value to a string
void yaml_pretty_print_to_string(Yaml *value, int indent, char **buffer, size_t *size)
{
    for (int i = 0; i < indent; i++)
        append_to_buffer(buffer, size, "  ");
    switch (value->type)
    {
    case SCALAR:
        append_to_buffer(buffer, size, "%s\n", value->data.scalar_value);
        break;
    case SEQUENCE:
        for (size_t i = 0; i < value->data.sequence_value.size; i++)
        {
            for (int j = 0; j < indent; j++)
                append_to_buffer(buffer, size, "  ");
            append_to_buffer(buffer, size, "- ");
            yaml_pretty_print_to_string(value->data.sequence_value.items[i], indent + 1, buffer, size);
        }
        break;
    case MAPPING:
        for (size_t i = 0; i < value->data.mapping_value.size; i++)
        {
            for (int j = 0; j < indent; j++)
                append_to_buffer(buffer, size, "  ");
            append_to_buffer(buffer, size, "%s: ", value->data.mapping_value.items[i]->key);
            yaml_pretty_print_to_string(value->data.mapping_value.items[i]->value, indent + 1, buffer, size);
        }
        break;
    }
}

// Function to write a string to a file
int write_string_to_file(const char *filename, const char *content)
{
    FILE *file = fopen(filename, "w");
    if (!file)
    {
        perror("Failed to open file");
        return -1;
    }

    fprintf(file, "%s", content);
    fclose(file);
    return 0;
}