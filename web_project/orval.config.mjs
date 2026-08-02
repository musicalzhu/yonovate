export default {
  esp32: {
    input: {
      target: './openapi.yaml',
    },
    output: {
      target: './src/generated/api.ts',
      client: 'vue-query',
      mode: 'single',
      clean: true,
      override: {
        mutator: {
          path: './src/services/mutator.ts',
          name: 'customInstance',
        },
        query: {
          useQuery: true,
          useMutation: true,
          useBaseQuery: true,
          queryKey: true,
        },
      },
    },
  },
};
