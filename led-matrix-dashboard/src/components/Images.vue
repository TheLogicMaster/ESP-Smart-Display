<template>
  <div>
    <div class="md-layout md-alignment-space-between-left">
      <md-button class="md-accent md-raised md-layout-item" @click="refreshImages(true)">Refresh</md-button>
      <md-button class="md-accent md-raised md-layout-item" @click="createImageDialog = true">Create Image</md-button>
      <input type="file" hidden ref="importFile" @change="onSelectImport">
      <md-button class="md-accent md-raised md-layout-item" @click="$refs.importFile.click()">Import Image</md-button>
    </div>
    <div>
      <md-card v-for="(data, image) in this.$store.state.imageData" v-bind:key="image">
        <md-card-header class="md-title">{{ image }}</md-card-header>
        <md-card-media>
          <image-preview :interval="0" :frame="0" :frames="1" :progress-bar="true" :pixel-size="pixelSize" :image="image" :canvas-height="pixelSize * data.height" :canvas-width="pixelSize * data.width"></image-preview>
        </md-card-media>
        <md-card-actions md-alignment="space-between">
          <md-button class="md-accent md-raised" @click="getImage(image)">Reload</md-button>
          <md-button class="md-accent md-raised" :href="'#/edit_image/' + image">Edit</md-button>
          <md-button :disabled="data.progmem" class="md-accent md-raised" @click="deleteImageClick(image)">Delete</md-button>
        </md-card-actions>
      </md-card>
    </div>

    <md-dialog :md-active.sync="createImageDialog">
      <md-dialog-title>Create Image</md-dialog-title>
      <md-field class="field">
        <label>Image Name</label>
        <md-input v-model="name"></md-input>
      </md-field>
      <div class="field">
        <label class="label">Image Width</label>
        <input class="" min="1" :max="displayWidth" type="number" v-model.number="width">
      </div>
      <div class="field">
        <label class="label">Image Height</label>
        <input class="" min="1" :max="displayHeight" type="number" v-model.number="height">
      </div>
      <div class="field">
        <label class="label">Image Frames</label>
        <input class="" min="1" max="100" type="number" v-model.number="frames">
      </div>

      <md-dialog-actions>
        <md-button class="md-accent md-raised" @click="createImageDialog = false">Close</md-button>
        <md-button class="md-accent md-raised" @click="createImage">Create</md-button>
      </md-dialog-actions>
    </md-dialog>

    <md-dialog-confirm
        :md-active.sync="deleteConfirm"
        md-title="Delete Image"
        :md-content="'Are you sure you want to delete ' + deleteImageName + '?'"
        md-confirm-text="Delete"
        md-cancel-text="Cancel"
        @md-confirm="deleteTheImage" />
  </div>
</template>

<script>
import ImagePreview from "@/components/ImagePreview";
export default {
  name: "Images",
  components: {ImagePreview},
  async mounted() {
    await this.waitForStats()
    this.displayWidth = this.$store.state.stats.width
    this.width = this.displayWidth
    this.displayHeight = this.$store.state.stats.height
    this.height = this.displayHeight
    await this.waitForPromiseSuccess(this.getStats, 2000)
    await this.refreshImages(Object.keys(this.$store.state.images).length === 0)
  },
  data: () => ({
    deleteConfirm: false,
    createImageDialog: false,
    deleteImageName: '',
    displayWidth: 64,
    displayHeight: 32,
    pixelSize: 3,
    frames: 1,
    width: 1,
    height: 1,
    name: '',
    importFile: null
  }),
  methods: {
    onSelectImport(file) {
      this.importFile = file[0]
    },
    async createImage() {
      let buffer = new Uint16Array(this.width * this.height * this.frames).fill(this.pack565Color({r: 255, g: 0, b: 0}))
      this.createImageDialog = false
      await this.saveImageBuffer(this.name, {width: this.width, height: this.height, length: this.frames, type: 1}, buffer)
      this.name = ''
      this.width = this.displayWidth
      this.height = this.displayHeight
      this.frames = 1
      await this.getImageData()
    },
    deleteImageClick(image) {
      this.deleteImageName = image
      this.deleteConfirm = true
    },
    async deleteTheImage() {
      await this.deleteImage(this.deleteImageName)
      await this.getImageData()
    }
  }
}
</script>

<style scoped>
.md-card {
  width: 300px;
  margin: 4px;
  display: inline-block;
  vertical-align: top;
}

.field {
  margin-left: 15px;
  width: 250px;
  //margin-right: 15px;
  margin-top: 15px;
  display: flex;
}

.label {
  width: 120px;
  display: flex;
}
</style>
