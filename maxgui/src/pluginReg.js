import Vue from 'vue'
import Vuetify from 'vuetify/lib'
import helpersPlugin from '@share/plugins/helpers'
import logger from '@share/plugins/logger'
import typy from '@src/plugins/typy'
import shortkey from '@src/plugins/shortkey'
import scopingI18n from '@share/plugins/scopingI18n'
import txtHighlighter from '@share/plugins/txtHighlighter'
import Vuex from 'vuex'
import PortalVue from 'portal-vue'
import VueI18n from 'vue-i18n'
import Workspace from '@src/plugins/workspace.js'
import Ripple from 'vuetify/lib/directives/ripple'

Vue.use(VueI18n)
// i18n only available after Vue.use(VueI18n)
Vue.use(scopingI18n, { i18n: require('@src/plugins/i18n').default })
// Workaround Ripple issue for v-simple-checkbox https://github.com/vuetifyjs/vuetify/issues/12224
Vue.use(Vuetify, { directives: { Ripple } })
Vue.use(helpersPlugin)
Vue.use(typy)
Vue.use(shortkey)
Vue.use(logger)
Vue.use(txtHighlighter)
Vue.use(Vuex)
// portal-value isn't needed in test env
if (process.env.NODE_ENV !== 'test') Vue.use(PortalVue)
// store only available after Vue.use(Vuex), so here use require for importing es module
const store = require('./store/index').default
Vue.use(require('@src/plugins/http').default, { store })
Vue.use(Workspace, { store })
