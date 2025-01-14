#include <string.h>
#include "dds/ddsrt/heap.h"
#include "dds/ddsrt/string.h"
#include "dds/ddsrt/strtol.h"
#include "dds/ddsi/ddsi_locator.h"
#include "dds/ddsc/dds_psmx.h"
#include "dds__psmx.h"
#include "psmx_dummy_v0_public.h"
#include "psmx_dummy_v0_impl.h"

static dummy_mockstats_t g_mockstats;

dummy_mockstats_t* dummy_v0_mockstats_get_ptr(void)
{
  return &g_mockstats;
}

static void dummy_psmx_v0_loan_free(dds_loaned_sample_t* loan)
{
  for (size_t l = 0; l < g_mockstats.loans._length; l++)
  {
    void **b = (void**)g_mockstats.loans._buffer;
    if (b[l] == loan->sample_ptr)
    {
      ddsrt_free(b[l]);
      b[l] = NULL;
    }
  }
}

static dds_loaned_sample_t* dummy_psmx_v0_ep_request_loan(dds_psmx_endpoint_v0_t* psmx_endpoint, uint32_t size_requested)
{
  if (g_mockstats.loans._length == g_mockstats.loans._maximum)
    return NULL;
  ++g_mockstats.cnt_request_loan;
  g_mockstats.request_loan_rcv_endpt = (dds_psmx_endpoint_t *) psmx_endpoint;
  memset(&g_mockstats.loan, 0x0, sizeof(dds_loaned_sample_t));
  memset(&g_mockstats.loan_metadata, 0x0, sizeof(dds_psmx_metadata_t));
  g_mockstats.loan.ops.free = dummy_psmx_v0_loan_free;
  g_mockstats.loan.loan_origin.origin_kind = DDS_LOAN_ORIGIN_KIND_PSMX;
  g_mockstats.loan.loan_origin.psmx_endpoint = (dds_psmx_endpoint_t *) psmx_endpoint;
  g_mockstats.loan.metadata = &g_mockstats.loan_metadata;
  g_mockstats.loan.sample_ptr = ddsrt_malloc(size_requested);
  ddsrt_atomic_st32 (&g_mockstats.loan.refc, 1);
  void **b = (void**)g_mockstats.loans._buffer;
  b[g_mockstats.loans._length++] = g_mockstats.loan.sample_ptr;
  return &g_mockstats.loan;
}

static dds_return_t dummy_psmx_v0_ep_write(dds_psmx_endpoint_v0_t* psmx_endpoint, dds_loaned_sample_t* data)
{
  (void)psmx_endpoint;
  (void)data;
  // Details yet to be implemented
  ++g_mockstats.cnt_write;
  g_mockstats.write_rcv_endpt = (dds_psmx_endpoint_t *) psmx_endpoint;
  g_mockstats.write_rcv_loan = data;
  return DDS_RETCODE_OK;
}

static dds_loaned_sample_t* dummy_psmx_v0_ep_take(dds_psmx_endpoint_v0_t* psmx_endpoint)
{
  (void)psmx_endpoint;
  // Details yet to be implemented
  ++g_mockstats.cnt_take;
  return NULL;
}

static dds_return_t dummy_psmx_v0_ep_on_data_available(dds_psmx_endpoint_v0_t* psmx_endpoint, dds_entity_t reader)
{
  (void)psmx_endpoint;
  (void)reader;
  // Details yet to be implemented
  ++g_mockstats.cnt_on_data_available;
  return DDS_RETCODE_OK;
}

static dds_psmx_endpoint_v0_t* dummy_psmx_v0_create_endpoint(
  dds_psmx_topic_t* psmx_topic,
  const struct dds_qos* qos,
  dds_psmx_endpoint_type_t endpoint_type
) {
  (void)qos;
  dds_psmx_endpoint_t* endpv1 = (dds_psmx_endpoint_t*)g_mockstats.endpoints._buffer + g_mockstats.endpoints._length++;
  dds_psmx_endpoint_v0_t *endp = (dds_psmx_endpoint_v0_t *)endpv1;
  DDSRT_STATIC_ASSERT (sizeof (*endp) <= sizeof (*endpv1));
  memset(endp, 0, sizeof(dds_psmx_endpoint_v0_t));
  endp->ops.request_loan = (dds_psmx_endpoint_request_loan_fn) dummy_psmx_v0_ep_request_loan;
  endp->ops.write = (dds_psmx_endpoint_write_fn) dummy_psmx_v0_ep_write;
  endp->ops.take = (dds_psmx_endpoint_take_fn) dummy_psmx_v0_ep_take;
  endp->ops.on_data_available = (dds_psmx_endpoint_on_data_available_fn) dummy_psmx_v0_ep_on_data_available;

  endp->psmx_topic = psmx_topic;
  endp->endpoint_type = endpoint_type;

  ++g_mockstats.cnt_create_endpoint;
  g_mockstats.create_endpoint_rcv_topic = psmx_topic;
  return endp;
}

static dds_return_t dummy_psmx_v0_delete_endpoint(dds_psmx_endpoint_v0_t* psmx_endpoint)
{
  memset(psmx_endpoint, 0x0, sizeof(dds_psmx_endpoint_v0_t));
  ++g_mockstats.cnt_delete_endpoint;
  g_mockstats.delete_endpoint_rcv_endpt = (dds_psmx_endpoint_t *) psmx_endpoint;
  return DDS_RETCODE_OK;
}

static bool dummy_psmx_v0_type_qos_supported(
  dds_psmx_v0_t* psmx,
  dds_psmx_endpoint_type_t forwhat,
  dds_data_type_properties_t data_type_props,
  const struct dds_qos* qos
) {
  (void)psmx;
  (void)forwhat;
  (void)data_type_props;
  (void)qos;
  ++g_mockstats.cnt_type_qos_supported;
  return true;
}

static dds_psmx_topic_t* dummy_psmx_v0_create_topic(
  dds_psmx_v0_t* psmx,
  const char* topic_name,
  const char* type_name,
  dds_data_type_properties_t data_type_props
) {
  (void)data_type_props;
  assert(g_mockstats.topics._length < g_mockstats.topics._maximum);
  dds_psmx_topic_t* topic = (dds_psmx_topic_t*)g_mockstats.topics._buffer + g_mockstats.topics._length++;
  memset(topic, 0, sizeof(dds_psmx_topic_t));
  topic->ops.create_endpoint = (dds_psmx_create_endpoint_fn)dummy_psmx_v0_create_endpoint;
  topic->ops.delete_endpoint = (dds_psmx_delete_endpoint_fn)dummy_psmx_v0_delete_endpoint;
  topic->psmx_instance = (dds_psmx_t *) psmx;
  topic->topic_name = ddsrt_strdup(topic_name);
  topic->type_name = ddsrt_strdup(type_name);
  ++g_mockstats.cnt_create_topic;
  return topic;
}

static dds_return_t dummy_psmx_v0_delete_topic(dds_psmx_topic_t* psmx_topic)
{
  dds_psmx_topic_cleanup_generic(psmx_topic);
  memset(psmx_topic, 0x0, sizeof(dds_psmx_topic_t));
  ++g_mockstats.cnt_delete_topic;
  return DDS_RETCODE_OK;
}

static dds_return_t dummy_psmx_v0_deinit(dds_psmx_v0_t* psmx)
{
  dds_psmx_cleanup_generic((dds_psmx_t *) psmx);
  dds_free(psmx);
  ++g_mockstats.cnt_deinit;
  ddsrt_free(g_mockstats.config);
  ddsrt_free(g_mockstats.topics._buffer);
  ddsrt_free(g_mockstats.endpoints._buffer);
  for (size_t l = 0; l < g_mockstats.loans._length; l++)
  {
    void **b = (void**)g_mockstats.loans._buffer;
    if (b[l] != NULL)
      ddsrt_free(b[l]);
    b[l] = NULL;
  }
  ddsrt_free(g_mockstats.loans._buffer);
  return DDS_RETCODE_OK;
}

static dds_psmx_node_identifier_t dummy_psmx_v0_get_node_id(const dds_psmx_v0_t* psmx)
{
  (void)psmx;
  dds_psmx_node_identifier_t node_id;
  memset(&node_id, 0, sizeof(dds_psmx_node_identifier_t));
  ++g_mockstats.cnt_get_node_id;
  return node_id;
}

static dds_psmx_features_t dummy_supported_features(const dds_psmx_t* psmx)
{
  (void)psmx;
  ++g_mockstats.cnt_supported_features;
  return g_mockstats.supports_shared_memory ? (DDS_PSMX_FEATURE_SHARED_MEMORY | DDS_PSMX_FEATURE_ZERO_COPY) : 0;
}

dds_return_t dummy_v0_create_psmx(dds_psmx_v0_t** psmx_out, dds_psmx_instance_id_t instance_id, const char* config)
{
  assert(psmx_out);
  memset(&g_mockstats, 0, sizeof(dummy_mockstats_t));
  g_mockstats.cnt_create_psmx = 1;

  dds_psmx_v0_t* psmx = dds_alloc(sizeof(*psmx));
  memset(psmx, 0, sizeof (*psmx));
  psmx->instance_name = dds_psmx_get_config_option_value(config, "SERVICE_NAME");
  if (psmx->instance_name == NULL)
    dds_string_dup("dummy_psmx");

  psmx->instance_id = instance_id;

  psmx->ops.type_qos_supported = (dds_psmx_type_qos_supported_fn) dummy_psmx_v0_type_qos_supported;
  psmx->ops.create_topic = (dds_psmx_create_topic_fn) dummy_psmx_v0_create_topic;
  psmx->ops.delete_topic = (dds_psmx_delete_topic_fn) dummy_psmx_v0_delete_topic;
  psmx->ops.deinit = (dds_psmx_deinit_fn) dummy_psmx_v0_deinit;
  psmx->ops.get_node_id = (dds_psmx_get_node_identifier_fn) dummy_psmx_v0_get_node_id;
  psmx->ops.supported_features = (dds_psmx_supported_features_fn) dummy_supported_features;
  dds_psmx_init_generic((dds_psmx_t *) psmx);

  g_mockstats.config = ddsrt_strdup(config);

  *psmx_out = psmx;
  return DDS_RETCODE_OK;
}
