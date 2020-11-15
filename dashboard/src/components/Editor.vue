<template>
  <div class="md-layout md-alignment-center-center">
    <md-card class="json-editor md-layout-item">
      <h1>Configuration Editor</h1>
      <div class="md-layout">
        <div class="md-layout-item">
          <md-button class="wide md-accent md-raised" @click="manualReload">Reload</md-button>
          <md-tooltip md-delay="1000" md-direction="bottom"> Reload display configuration </md-tooltip>
        </div>
        <div class="md-layout-item">
          <md-button class="wide md-accent md-raised" @click="manualSave">Save</md-button>
          <md-tooltip md-delay="1000" md-direction="bottom"> Save display configuration </md-tooltip>
        </div>
        <div class="md-layout-item">
          <md-checkbox class="md-accent" v-model="autoSave" @change="changeAutoSave">Auto-Save</md-checkbox>
          <md-tooltip md-delay="1000" md-direction="bottom"> Enable auto-saving with each change </md-tooltip>
        </div>
      </div>
      <JsonEditor
          :options="{
            confirmText: 'Confirm',
            cancelText: 'Cancel',
        }"
          :objData="jsonData"
          v-model="jsonData">
      </JsonEditor>
    </md-card>
  </div>
</template>

<script>
export default {
  name: "Editor",
  data: () => ({
    jsonData: {},
    autoSave: false
  }),
  watch: {
    'jsonData': function (jsonData) {
      if (this.autoSave)
        this.save(jsonData)
    }
  },
  methods: {
    async changeAutoSave() {
      if (this.autoSave) {
        if (await this.confirm('Auto-Save', 'Are you sure you want to enable autosaving? This will instantly overwrite the display configuration upon config changes.'))
          await this.save(this.jsonData)
        else
          this.autoSave = false
      }
    },
    async manualSave() {
      if (await this.confirm('Save Config', 'Are you sure you want to overwrite the current configuration?'))
        await this.save(this.jsonData)
    },
    async save(jsonData) {
      await this.saveConfig(jsonData)
    },
    async manualReload() {
      if (this.areObjectsEqual(this.$store.state.configuration, this.jsonData) || (await this.confirm('Reload Config', 'Are you sure you want to reload the configuration file? Unsaved changes will be lost.')))
        await this.reload()
    },
    async reload() {
      this.autoSave = false
      await this.getConfig()
      this.jsonData = this.cloneObject(this.$store.state.configuration)
    }
  },
  async mounted() {
    await this.reload()
  }
}
</script>

<style scoped>
.json-editor {
  max-width: 500px;
}
</style>
