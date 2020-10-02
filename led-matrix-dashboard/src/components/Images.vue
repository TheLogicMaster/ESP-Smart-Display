<template>
  <div>
    <div class="md-layout md-alignment-space-between-left">
      <md-button class="md-accent md-raised md-layout-item" @click="refreshImages(true)">Refresh</md-button>
      <md-button class="md-accent md-raised md-layout-item" @click="openCreateDialog">Create Image</md-button>
      <input type="file" hidden ref="importFile" @input="onSelectImport">
      <md-button class="md-accent md-raised md-layout-item" @click="$refs.importFile.click()">Import Image</md-button>
    </div>
    <div>
      <md-card v-for="(data, image) in this.$store.state.imageData" v-bind:key="image">
        <md-card-header class="md-title">
          {{ image }}
          <md-button v-if="!$store.state.imageData[image].progmem" class="md-icon-button" @click="openRenameDialog(image)">
            <md-icon>create</md-icon>
          </md-button>
        </md-card-header>
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
      <md-field class="field" :class="{'md-invalid': !nameValid}">
        <label>Image Name</label>
        <md-input :maxlength="this.$store.state.stats['maxPathLength'] - 8" v-model="name" @change="changeName"></md-input>
        <span class="md-error">Only alphanumeric and spaces</span>
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

    <md-dialog :md-active.sync="renameDialog">
      <md-dialog-title>Rename {{ renameName }}</md-dialog-title>
      <md-field class="field" :class="{'md-invalid': !renameNameValid}">
        <label>New Name</label>
        <md-input :maxlength="this.$store.state.stats['maxPathLength'] - 8" v-model="newName" @change="changeRenameName"></md-input>
        <span class="md-error">Only alphanumeric and spaces</span>
      </md-field>
      <md-dialog-actions>
        <md-button class="md-accent md-raised" @click="renameDialog = false">Close</md-button>
        <md-button class="md-accent md-raised" @click="rename">Rename</md-button>
      </md-dialog-actions>
    </md-dialog>

    <md-dialog :md-active.sync="importImageDialog">
      <md-dialog-title>Create Image</md-dialog-title>
      <md-field class="field" :class="{'md-invalid': !nameValid}">
        <label>Image Name</label>
        <md-input :maxlength="this.$store.state.stats['maxPathLength'] - 8" v-model="name" @change="changeName"></md-input>
        <span class="md-error">Only alphanumeric and spaces</span>
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
        <md-button class="md-accent md-raised" @click="importImageDialog = false">Close</md-button>
        <md-button class="md-accent md-raised" @click="importImage">Create</md-button>
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
    await this.waitForPromiseSuccess(this.getStats)
    this.width = this.displayWidth = this.$store.state.stats.width
    this.height = this.displayHeight = this.$store.state.stats.height
    let full = Object.keys(this.$store.state.images).length === 0
    if (!full)
      await this.waitForPromiseSuccess(this.getImageData)
    await this.refreshImages(full)
  },
  data: () => ({
    deleteConfirm: false,
    createImageDialog: false,
    importImageDialog: false,
    renameDialog: false,
    deleteImageName: '',
    displayWidth: 64,
    displayHeight: 32,
    pixelSize: 3,
    frames: 1,
    width: 1,
    height: 1,
    name: '',
    nameValid: true,
    importFile: null,
    renameName: '',
    newName: '',
    renameNameValid: true
  }),
  methods: {
    changeRenameName() {
      this.renameNameValid = this.validateFilename(this.newName)
    },
    changeName() {
      this.nameValid = this.validateFilename(this.name)
    },
    async openCreateDialog() {
      this.createImageDialog = true
      this.name = ''
      this.name = await this.getRandomName()
    },
    openRenameDialog(image) {
      this.renameDialog = true
      this.renameName = image
      this.newName = image
      this.renameNameValid = true
    },
    async rename() {
      if (!this.validateFilename(this.newName))
        return
      this.renameDialog = false
      if (this.newName === this.renameName)
        return
      await this.renameImage(this.renameName, this.newName)
    },
    async onSelectImport(event) {
      if (event.target.files === 0)
        return
      this.importFile = event.target.files[0]
      this.$refs.importFile.value = ''
      if (this.importFile.name.endsWith('.bin')) {
        let info = this.parseBinaryFilename(this.importFile.name)
        this.width = info.width
        this.height = info.height
        this.frames = info.frames
        this.name = info.name
      } else {
        this.frames = 1
        let image = await this.$jimp.create(await this.importFile.arrayBuffer())
        let ratio = Math.min(this.$store.state.stats.height / image.getHeight(), this.$store.state.stats.width / image.getWidth())
        this.width = Math.round(image.getWidth() * ratio)
        this.height = Math.round(image.getHeight() * ratio)
        this.name = this.importFile.name.split('.')[0]
        this.name = this.name.substr(0, Math.min(this.$store.state.stats['maxPathLength'] - 8, this.name.length))
      }
      this.importImageDialog = true
    },
    async importImage(){
      let info = {
        width: this.width,
        height: this.height,
        length: this.frames
      }
      if (this.importFile.name.endsWith('.bin')) {
        await this.saveImageBuffer(this.name, info, await this.importFile.arrayBuffer())
      } else {
        let buffer = new Uint16Array(this.width * this.height * this.frames)
        let image = await this.$jimp.create(await this.importFile.arrayBuffer())
        await image.resize(this.width, this.height)
        for (let y = 0; y < this.height; y++)
          for (let x = 0; x < this.width; x++)
            buffer[this.width * y + x] = this.pack565Color(this.$jimp.intToRGBA(await image.getPixelColor(x, y)))
        await this.saveImageBuffer(this.name, info, buffer)
      }
      this.importImageDialog = false
      this.name = ''
      this.width = this.displayWidth
      this.height = this.displayHeight
      this.frames = 1
      await this.getImageData()
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
