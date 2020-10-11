<template>
  <div class="md-layout md-alignment-center-center">
    <md-card class="json-editor md-layout-item">
      <h1>Configuration Editor</h1>
      <div class="md-layout">
        <md-button class="md-layout-item md-accent md-raised" @click="manualReload">Reload</md-button>
        <md-button class="md-layout-item md-accent md-raised" @click="manualSave">Save</md-button>
        <md-checkbox class="md-layout-item md-accent" v-model="autoSave" @change="changeAutoSave">Auto-Save</md-checkbox>
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
