<template>
  <div class="md-layout md-alignment-center-center">
    <md-card class="json-editor md-layout-item">
      <h1>Configuration Editor</h1>
      <div class="md-layout">
        <md-button class="md-layout-item md-accent md-raised" @click="buttonReload">Reload</md-button>
        <md-button class="md-layout-item md-accent md-raised" @click="buttonSave">Save</md-button>
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
    changeAutoSave() {
      if (this.autoSave) {
        if (window.confirm("Are you sure you want to enable autosaving? This will instantly overwrite the display configuration upon config changes."))
          this.save(this.jsonData)
        else
          this.autoSave = false
      }
    },
    buttonReload() {
      if (window.confirm('Are you sure you want to reload the configuration file? Unsaved changes will be lost.'))
        this.reload()
    },
    buttonSave() {
      if (window.confirm('Are you sure you want to overwrite the current configuration?'))
        this.save(this.jsonData)
    },
    async save(jsonData) {
      await this.saveConfig(jsonData)
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
